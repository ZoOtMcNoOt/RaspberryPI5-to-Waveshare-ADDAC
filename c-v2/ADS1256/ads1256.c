/*
	Author: Jure Bartol (Original), Modified for libgpiod/RPi5 support
	Date: 07.05.2016 (Original), Modified: 2025
	
	Modified to use libgpiod instead of bcm2835 for Raspberry Pi 5 support
	
	Build example: gcc ads1256_libgpiod.c -std=c99 -o ads1256 -lgpiod
*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <gpiod.h>
#include <errno.h>
#include <string.h>

/*
	***************************
	** PART 0 - enumerations **
	***************************
*/

// Set custom data types that are 8, 16 and 32 bits long.
#define uint8_t  unsigned char  	// 1 byte
#define uint16_t unsigned short 	// 2 bytes
#define uint32_t unsigned long  	// 4 bytes

//  Set the Programmable gain amplifier (PGA).
enum
{
	PGA_GAIN1	= 0, // Input voltage range: +- 5 V
	PGA_GAIN2	= 1, // Input voltage range: +- 2.5 V
	PGA_GAIN4	= 2, // Input voltage range: +- 1.25 V
	PGA_GAIN8	= 3, // Input voltage range: +- 0.625 V
	PGA_GAIN16	= 4, // Input voltage range: +- 0.3125 V
	PGA_GAIN32	= 5, // Input voltage range: +- 0.15625 V
	PGA_GAIN64	= 6  // Input voltage range: +- 0.078125 V
};

//  Set a data rate of a programmable filter (programmable averager).
enum
{
	DRATE_30000 = 0xF0, 
	DRATE_15000 = 0xE0,
	DRATE_7500  = 0xD0,
	DRATE_3750  = 0xC0,
	DRATE_2000  = 0xB0,
	DRATE_1000  = 0xA1,
	DRATE_500   = 0x92,
	DRATE_100   = 0x82,
	DRATE_60    = 0x72,
	DRATE_50    = 0x63,
	DRATE_30    = 0x53,
	DRATE_25    = 0x43,
	DRATE_15    = 0x33,
	DRATE_10    = 0x20,
	DRATE_5     = 0x13,
	DRATE_2d5   = 0x03
};

//  Set of registers.
enum
{
	REG_STATUS = 0,	 // Register adress: 00h, Reset value: x1H
	REG_MUX    = 1,  // Register adress: 01h, Reset value: 01H
	REG_ADCON  = 2,  // Register adress: 02h, Reset value: 20H
	REG_DRATE  = 3,  // Register adress: 03h, Reset value: F0H
	REG_IO     = 4,  // Register adress: 04h, Reset value: E0H
	REG_OFC0   = 5,  // Register adress: 05h, Reset value: xxH
	REG_OFC1   = 6,  // Register adress: 06h, Reset value: xxH
	REG_OFC2   = 7,  // Register adress: 07h, Reset value: xxH
	REG_FSC0   = 8,  // Register adress: 08h, Reset value: xxH
	REG_FSC1   = 9,  // Register adress: 09h, Reset value: xxH
	REG_FSC2   = 10, // Register adress: 0Ah, Reset value: xxH
};

//  Commands for controlling operation of the ADS1256.
enum
{
	CMD_WAKEUP   = 0x00, // Completes SYNC and Exits Standby Mode
	CMD_RDATA    = 0x01, // Read Data
	CMD_RDATAC   = 0x03, // Read Data Continuously
	CMD_SDATAC   = 0x0F, // Stop Read Data Continuously
	CMD_RREG     = 0x10, // Read from REG
	CMD_WREG     = 0x50, // Write to REG
	CMD_SELFCAL  = 0xF0, // Offset and Gain Self-Calibration
	CMD_SELFOCAL = 0xF1, // Offset Self-Calibration
	CMD_SELFGCAL = 0xF2, // Gain Self-Calibration
	CMD_SYSOCAL  = 0xF3, // System Offset Calibration
	CMD_SYSGCAL  = 0xF4, // System Gain Calibration
	CMD_SYNC     = 0xFC, // Synchronize the A/D Conversion
	CMD_STANDBY  = 0xFD, // Begin Standby Mode
	CMD_RESET    = 0xFE, // Reset to Power-Up Values
};

// Input analog channels.
enum
{
	AIN0   = 0, //Binary value: 0000 0000
	AIN1   = 1, //Binary value: 0000 0001
	AIN2   = 2, //Binary value: 0000 0010
	AIN3   = 3, //Binary value: 0000 0011
	AIN4   = 4, //Binary value: 0000 0100
	AIN5   = 5, //Binary value: 0000 0101
	AIN6   = 6, //Binary value: 0000 0110
	AIN7   = 7, //Binary value: 0000 0111
	AINCOM = 8  //Binary value: 0000 1000
};

// Boolean values.
typedef enum
{
	False = 0,
	True  = 1,
} bool;

/*
	*******************************
	** PART 1 - serial interface **
	*******************************
*/

// GPIO pin definitions (BCM numbering)
#define DRDY_PIN	17  // GPIO17 (Physical pin 11)
#define RST_PIN		18  // GPIO18 (Physical pin 12)
#define CS_PIN		22  // GPIO22 (Physical pin 15)

// SPI device
#define SPI_DEVICE	"/dev/spidev0.0"

// Global variables for GPIO and SPI
static struct gpiod_chip *chip;
static struct gpiod_line *drdy_line;
static struct gpiod_line *rst_line;
static struct gpiod_line *cs_line;
static int spi_fd = -1;

// SPI configuration
static uint8_t spi_mode = SPI_MODE_1;
static uint8_t spi_bits = 8;
static uint32_t spi_speed = 976562; // ~1MHz (250MHz/256)

// Set CS to high
#define CS_1() gpiod_line_set_value(cs_line, 1)
// Set CS to low
#define CS_0() gpiod_line_set_value(cs_line, 0)
// Set RST to high
#define RST_1() gpiod_line_set_value(rst_line, 1)
// Set RST to low
#define RST_0() gpiod_line_set_value(rst_line, 0)
// Returns True if DRDY is low
#define DRDY_LOW() (gpiod_line_get_value(drdy_line) == 0)

// Delay in microseconds.
void delayus(uint64_t microseconds)
{
	usleep(microseconds);
}

// Send 8 bit value over SPI interface.
void send8bit(uint8_t data)
{
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)&data,
		.rx_buf = 0,
		.len = 1,
		.delay_usecs = 0,
		.speed_hz = spi_speed,
		.bits_per_word = spi_bits,
	};

	if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
		perror("SPI send failed");
	}
}

// Receive 8 bit value over SPI interface.
uint8_t recieve8bit(void)
{
	uint8_t tx_data = 0xFF;
	uint8_t rx_data = 0;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)&tx_data,
		.rx_buf = (unsigned long)&rx_data,
		.len = 1,
		.delay_usecs = 0,
		.speed_hz = spi_speed,
		.bits_per_word = spi_bits,
	};

	if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
		perror("SPI receive failed");
		return 0;
	}

	return rx_data;
}

// Wait until DRDY is low.
void waitDRDY(void)
{
	while(!DRDY_LOW()) {
		usleep(1);
	}
}

// Initialize GPIO and SPI
uint8_t initializeSPI()
{
	// Open GPIO chip
	chip = gpiod_chip_open_by_name("gpiochip4"); // RPi5 uses gpiochip4
	if (!chip) {
		chip = gpiod_chip_open_by_name("gpiochip0"); // Fallback for older RPi
		if (!chip) {
			fprintf(stderr, "Failed to open GPIO chip\n");
			return 0;
		}
	}

	// Get GPIO lines
	drdy_line = gpiod_chip_get_line(chip, DRDY_PIN);
	rst_line = gpiod_chip_get_line(chip, RST_PIN);
	cs_line = gpiod_chip_get_line(chip, CS_PIN);

	if (!drdy_line || !rst_line || !cs_line) {
		fprintf(stderr, "Failed to get GPIO lines\n");
		gpiod_chip_close(chip);
		return 0;
	}

	// Request GPIO lines
	if (gpiod_line_request_input(drdy_line, "ADS1256_DRDY") < 0) {
		fprintf(stderr, "Failed to request DRDY line\n");
		gpiod_chip_close(chip);
		return 0;
	}

	if (gpiod_line_request_output(rst_line, "ADS1256_RST", 1) < 0) {
		fprintf(stderr, "Failed to request RST line\n");
		gpiod_line_release(drdy_line);
		gpiod_chip_close(chip);
		return 0;
	}

	if (gpiod_line_request_output(cs_line, "ADS1256_CS", 1) < 0) {
		fprintf(stderr, "Failed to request CS line\n");
		gpiod_line_release(drdy_line);
		gpiod_line_release(rst_line);
		gpiod_chip_close(chip);
		return 0;
	}

	// Open SPI device
	spi_fd = open(SPI_DEVICE, O_RDWR);
	if (spi_fd < 0) {
		fprintf(stderr, "Failed to open SPI device %s: %s\n", SPI_DEVICE, strerror(errno));
		gpiod_line_release(drdy_line);
		gpiod_line_release(rst_line);
		gpiod_line_release(cs_line);
		gpiod_chip_close(chip);
		return 0;
	}

	// Configure SPI
	if (ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode) < 0 ||
		ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits) < 0 ||
		ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
		fprintf(stderr, "Failed to configure SPI\n");
		close(spi_fd);
		gpiod_line_release(drdy_line);
		gpiod_line_release(rst_line);
		gpiod_line_release(cs_line);
		gpiod_chip_close(chip);
		return 0;
	}

	// Set initial pin states
	CS_1();
	RST_1();

	return 1;
}

// Cleanup GPIO and SPI
void endSPI()
{
	if (spi_fd >= 0) {
		close(spi_fd);
		spi_fd = -1;
	}

	if (drdy_line) {
		gpiod_line_release(drdy_line);
		drdy_line = NULL;
	}
	if (rst_line) {
		gpiod_line_release(rst_line);
		rst_line = NULL;
	}
	if (cs_line) {
		gpiod_line_release(cs_line);
		cs_line = NULL;
	}

	if (chip) {
		gpiod_chip_close(chip);
		chip = NULL;
	}
}

/*
	*****************************
	** PART 2 - ads1256 driver **
	*****************************
*/

// Read 1 byte from register address registerID.
uint8_t readByteFromReg(uint8_t registerID)
{
	CS_0();
	send8bit(CMD_RREG | registerID);
	send8bit(0x00);
	
	delayus(7);
	uint8_t read = recieve8bit();
	CS_1();
	return read;
}

// Write value (1 byte) to register address registerID.
void writeByteToReg(uint8_t registerID, uint8_t value)
{
	CS_0();
	send8bit(CMD_WREG | registerID);
	send8bit(0x00);
	send8bit(value);
	CS_1();
}

// Send standalone commands to register.
uint8_t writeCMD(uint8_t command)
{
	CS_0();
	send8bit(command);
	CS_1();
	return 0;
}

// Set the internal buffer (True - enable, False - disable).
uint8_t setBuffer(bool val)
{
	CS_0();
	send8bit(CMD_WREG | REG_STATUS);
	send8bit(0x00);
	send8bit((0 << 3) | (1 << 2) | (val << 1));
	CS_1();
	return 0;
}

// Get data from STATUS register - chip ID information.
uint8_t readChipID(void)
{
	waitDRDY();
	uint8_t id = readByteFromReg(REG_STATUS);
	return (id >> 4);
}

// Write to MUX register - set channel to read from in single-ended mode.
void setSEChannel(uint8_t channel)
{
	writeByteToReg(REG_MUX, channel << 4 | 1 << 3);
}

// Write to MUX register - set channel to read from in differential mode.
void setDIFFChannel(uint8_t positiveCh, uint8_t negativeCh)
{
	writeByteToReg(REG_MUX, positiveCh << 4 | negativeCh);
}

// Write to A/D control register - set programmable gain amplifier (PGA).
void setPGA(uint8_t pga)
{
	writeByteToReg(REG_ADCON, pga);
}

// Write to A/D data rate register - set data rate.
void setDataRate(uint8_t drate)
{
	writeByteToReg(REG_DRATE, drate);
}

// Read 24 bit value from ADS1256.
int32_t readData(void)
{
	uint32_t read = 0;
	uint8_t buffer[3];

	CS_0();
	send8bit(CMD_RDATA);
	delayus(7);

	buffer[0] = recieve8bit();
	buffer[1] = recieve8bit();
	buffer[2] = recieve8bit();

	// construct 24 bit value
	read =  ((uint32_t)buffer[0] << 16) & 0x00FF0000;
	read |= ((uint32_t)buffer[1] << 8);
	read |= buffer[2];
	if (read & 0x800000){
		read |= 0xFF000000;
	}

	CS_1();

	return (int32_t)read;
}

// Get one single-ended analog input value.
int32_t getValSEChannel(uint8_t channel)
{
	int32_t read = 0;
	setSEChannel(channel);
	delayus(3);
	writeCMD(CMD_SYNC);
	delayus(3);
	writeCMD(CMD_WAKEUP);
	delayus(1);
	read = readData();
	return read;
}

// Get one differential analog input value.
int32_t getValDIFFChannel(uint8_t positiveCh, uint8_t negativeCh)
{
	int32_t read = 0;
	setDIFFChannel(positiveCh, negativeCh);
	delayus(3);
	writeCMD(CMD_SYNC);
	delayus(3);
	writeCMD(CMD_WAKEUP);
	delayus(1);
	read = readData();
	return read;
}

// Get single-ended analog input values from multiple channels.
void scanSEChannels(uint8_t channels[], uint8_t numOfChannels, uint32_t *values)
{
	for (int i = 0; i < numOfChannels; ++i){
		waitDRDY();
		values[i] = getValSEChannel(channels[i]);
	}
}

// Get differential analog input values from multiple channels.
void scanDIFFChannels(uint8_t positiveChs[], uint8_t negativeChs[], uint8_t numOfChannels, uint32_t *values)
{
	for (int i = 0; i < numOfChannels; ++i){
		waitDRDY();
		values[i] = getValDIFFChannel(positiveChs[i], negativeChs[i]);
	}
}

// Continuously acquire analog data from one single-ended analog input.
void scanSEChannelContinuous(uint8_t channel, uint32_t numOfMeasure, uint32_t *values, uint32_t *currentTime)
{
	uint8_t buffer[3];
	uint32_t read = 0;
	uint8_t del = 8;
	
	setSEChannel(channel);
	delayus(del);
	
	CS_0();
	waitDRDY();
	send8bit(CMD_RDATAC);
	delayus(del);
	
	clock_t startTime = clock();
	for (int i = 0; i < numOfMeasure; ++i)
	{
		waitDRDY();
		buffer[0] = recieve8bit();
		buffer[1] = recieve8bit();
		buffer[2] = recieve8bit();

		read  = ((uint32_t)buffer[0] << 16) & 0x00FF0000;
		read |= ((uint32_t)buffer[1] << 8);
		read |= buffer[2];
		if (read & 0x800000){
			read |= 0xFF000000;
		}
		values[i] = read;
		currentTime[i] = clock() - startTime;
		delayus(del);
	}
	
	waitDRDY();
	send8bit(CMD_SDATAC);
	CS_1();
}

// Continuously acquire analog data from one differential analog input.
void scanDIFFChannelContinuous(uint8_t positiveCh, uint8_t negativeCh, uint32_t numOfMeasure, uint32_t *values, uint32_t *currentTime)
{
	uint8_t buffer[3];
	uint32_t read = 0;
	uint8_t del = 8;

	setDIFFChannel(positiveCh, negativeCh);
	delayus(del);

	CS_0();
	waitDRDY();
	send8bit(CMD_RDATAC);
	delayus(del);

	clock_t startTime = clock();
	for (int i = 0; i < numOfMeasure; ++i)
	{
		waitDRDY();
		buffer[0] = recieve8bit();
		buffer[1] = recieve8bit();
		buffer[2] = recieve8bit();

		read  = ((uint32_t)buffer[0] << 16) & 0x00FF0000;
		read |= ((uint32_t)buffer[1] << 8);
		read |= buffer[2];
		if (read & 0x800000){
			read |= 0xFF000000;
		}
		values[i] = read;
		currentTime[i] = clock() - startTime;
		delayus(del);
	}

	waitDRDY();
	send8bit(CMD_SDATAC);
	CS_1();
}

/*
	********************************
	** PART 4 - Examples of usage **
	********************************
*/

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("Usage: %s <number of measurements>\n", argv[0]);
		return 1;
	}

	// Initialization and AD configuration
	if (!initializeSPI()) {
		fprintf(stderr, "Failed to initialize SPI and GPIO\n");
		return 1;
	}
	
	setBuffer(True);
	setPGA(PGA_GAIN1);
	setDataRate(DRATE_30000);

	/////////////////////////////////
	// Single-ended input channels //
	/////////////////////////////////

	clock_t start_SE, end_SE;
	int num_ch_SE = 1;
	int num_measure_SE = atoi(argv[1]);
	uint32_t values_SE [num_ch_SE];
	uint8_t channels_SE [1] = {AIN2};
	
	start_SE = clock();
	for (int i = 0; i < num_measure_SE; ++i){
		scanSEChannels(channels_SE, num_ch_SE, values_SE);
		printf("%i ", i+1);
		for (int ch = 0; ch < num_ch_SE; ++ch)
		{
			printf(" %f %i", (double)values_SE[ch]/1670000, (int)(clock() - start_SE));
		}
		printf("\n");
	}
	end_SE = clock();

	/////////////////////////////////
	// Differential input channels //
	/////////////////////////////////

	clock_t start_DIFF, end_DIFF;
	int num_ch_DIFF = 1;
	int num_measure_DIFF = atoi(argv[1]);
	uint32_t values_DIFF [num_ch_DIFF];
	uint8_t  posChannels [1] = {AIN2};
	uint8_t  negChannels [1] = {AINCOM};
	
	start_DIFF = clock();
	for (int i = 0; i < num_measure_DIFF; ++i){
		scanDIFFChannels(posChannels, negChannels, num_ch_DIFF, values_DIFF);
		printf("%i ", i+1);
		for (int ch = 0; ch < num_ch_DIFF; ++ch)
		{
			printf(" %f %i", (double)values_DIFF[ch]/1670000, (int)(clock() - start_DIFF));
		}
		printf("\n");
	}
	end_DIFF = clock();

	/////////////////////////////////////////
	// Single-ended input, continuous mode //
	/////////////////////////////////////////

	clock_t start_SE_CONT, end_SE_CONT;
	int num_measure_SE_CONT = atoi(argv[1])*30;
	uint32_t values_SE_CONT [num_measure_SE_CONT];
	uint32_t time_SE_CONT [num_measure_SE_CONT];
	
	start_SE_CONT = clock();
	scanSEChannelContinuous(AIN2, num_measure_SE_CONT, values_SE_CONT, time_SE_CONT);
	end_SE_CONT = clock();
	
	for (int i = 0; i < num_measure_SE_CONT; ++i){
		printf("%i %f %i\n", i+1, (float)values_SE_CONT[i]/1670000, (int)time_SE_CONT[i]);
	}

	/////////////////////////////////////////
	// Differential input, continuous mode //
	/////////////////////////////////////////

	clock_t start_DIFF_CONT, end_DIFF_CONT;
	int num_measure_DIFF_CONT = atoi(argv[1])*30;
	uint32_t values_DIFF_CONT [num_measure_DIFF_CONT];
	uint32_t time_DIFF_CONT [num_measure_DIFF_CONT];
	
	start_DIFF_CONT = clock();
	scanDIFFChannelContinuous(AIN2, AINCOM, num_measure_DIFF_CONT, values_DIFF_CONT, time_DIFF_CONT);
	end_DIFF_CONT = clock();
	
	for (int i = 0; i < num_measure_DIFF_CONT; ++i){
		printf("%i %f %i\n", i+1, (float)values_DIFF_CONT[i]/1670000, (int)time_DIFF_CONT[i]);
	}

	printf("Time for %i single-ended measurements on %i channels is %d microseconds (%5.1f SPS/channel).\n", 
		   num_measure_SE, num_ch_SE, (int)(end_SE - start_SE), (double)(num_measure_SE)/(end_SE - start_SE)*1e6);
	printf("Time for %i differential measurements on %i channels is %d microseconds (%5.1f SPS/channel).\n", 
		   num_measure_DIFF, num_ch_DIFF, (int)(end_DIFF - start_DIFF), (double)num_measure_DIFF/(end_DIFF - start_DIFF)*1e6);
	printf("Time for %i single-ended measurements in continuous mode is %d microseconds (%5.1f SPS).\n", 
		   num_measure_SE_CONT, (int)(end_SE_CONT - start_SE_CONT), (double)num_measure_SE_CONT/(end_SE_CONT - start_SE_CONT)*1e6);
	printf("Time for %i differential measurements in continuous mode is %d microseconds (%5.1f SPS).\n", 
		   num_measure_DIFF_CONT, (int)(end_DIFF_CONT - start_DIFF_CONT), (double)num_measure_DIFF_CONT/(end_DIFF_CONT - start_DIFF_CONT)*1e6);

	endSPI();
	return 0;
}