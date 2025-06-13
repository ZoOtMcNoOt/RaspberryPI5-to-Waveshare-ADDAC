/*
 * ADS1256 Driver Implementation
 *
 * This file contains the low-level logic for controlling the ADS1256 ADC.
 * It uses libgpiod for GPIO control and spidev for SPI communication, making
 * it suitable for modern Linux systems like the Raspberry Pi 5.
 *
 * To use this driver, compile it into an object file and link it with your
 * main application.
 *
 * Example compilation:
 * gcc -c ads1256.c -o ads1256.o
 * gcc main.c ads1256.o -o my_app -lgpiod
 *
 */

#include "ads1256.h" 

// System headers for the implementation
#include <gpiod.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <time.h>

//####################################################################################
//
//##    3. PRIVATE DEFINITIONS & STATIC VARIABLES
//
//####################################################################################

// Internal Register and Command definitions (not needed in public header)
typedef enum {
	REG_STATUS = 0,	REG_MUX    = 1, REG_ADCON  = 2, REG_DRATE  = 3,
	REG_IO     = 4, REG_OFC0   = 5, REG_OFC1   = 6, REG_OFC2   = 7,
	REG_FSC0   = 8, REG_FSC1   = 9, REG_FSC2   = 10
} ads1256_reg_t;

typedef enum {
	CMD_WAKEUP   = 0x00, CMD_RDATA    = 0x01, CMD_RDATAC   = 0x03,
	CMD_SDATAC   = 0x0F, CMD_RREG     = 0x10, CMD_WREG     = 0x50,
	CMD_SELFCAL  = 0xF0, CMD_SYNC     = 0xFC, CMD_RESET    = 0xFE
} ads1256_cmd_t;


// Driver-internal (static) variables for device handles
static int spi_fd = -1;
static struct gpiod_chip *gpio_chip = NULL;
static struct gpiod_line *rst_line = NULL;
static struct gpiod_line *cs_line = NULL;
static struct gpiod_line *drdy_line = NULL;

// --- Scan Mode Static Variables ---
#define MAX_SCAN_CHANNELS 8 // Maximum number of channels in a scan sequence
static ads1256_ain_t scan_channels[MAX_SCAN_CHANNELS];
static uint8_t num_configured_scan_channels = 0;
static uint8_t current_scan_channel_index = 0;

// GPIO configuration (BCM numbering)
#define GPIO_CHIP_NAME "gpiochip4"
#define RST_PIN   18  // Reset pin
#define CS_PIN    22  // Chip Select pin
#define DRDY_PIN  17  // Data Ready pin

// SPI device path
#define SPI_DEVICE "/dev/spidev0.0"

// GPIO control macros for internal use
#define CS_1()      if(cs_line) gpiod_line_set_value(cs_line, 1)
#define CS_0()      if(cs_line) gpiod_line_set_value(cs_line, 0)
#define RST_1() 	if(rst_line) gpiod_line_set_value(rst_line, 1)
#define RST_0() 	if(rst_line) gpiod_line_set_value(rst_line, 0)
#define DRDY_IS_LOW()  (drdy_line ? (gpiod_line_get_value(drdy_line) == 0) : 0)


//####################################################################################
//
//##    4. STATIC HELPER FUNCTIONS (DRIVER-PRIVATE)
//
//####################################################################################

static void delay_us(uint64_t microseconds) {
    struct timespec req;
    req.tv_sec = microseconds / 1000000;
    req.tv_nsec = (microseconds % 1000000) * 1000;
    while(nanosleep(&req, &req) == -1);
}

static void wait_for_drdy(void) {
    // Wait for DRDY to go low, with a timeout to prevent infinite loops
    for (int i = 0; i < 4000000; i++) {
        if (DRDY_IS_LOW()) {
            return;
        }
    }
    fprintf(stderr, "Warning: wait_for_drdy() timed out.\n");
}

static int spi_transfer(uint8_t *tx_buf, uint8_t *rx_buf, uint32_t len) {
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)rx_buf,
        .len = len,
    };
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        perror("SPI: SPI_IOC_MESSAGE failed");
        return -1;
    }
    return 0;
}

static void write_cmd(ads1256_cmd_t cmd) {
    CS_0();
    spi_transfer((uint8_t*)&cmd, NULL, 1);
    CS_1();
}

static void write_reg(ads1256_reg_t reg, uint8_t value) {
    uint8_t buffer[3] = {CMD_WREG | reg, 0x00, value};
    CS_0();
    spi_transfer(buffer, NULL, 3);
    CS_1();
}

static uint8_t read_reg(ads1256_reg_t reg) {
    uint8_t buffer[2] = {CMD_RREG | reg, 0x00};
    uint8_t read_val;
    CS_0();
    spi_transfer(buffer, NULL, 2);
    delay_us(7); // t6 delay
    spi_transfer(NULL, &read_val, 1);
    CS_1();
    return read_val;
}

static int32_t read_data_raw(void) {
    uint8_t cmd = CMD_RDATA;
    uint8_t buffer[3];
    uint32_t read_val = 0;

    CS_0();
    spi_transfer(&cmd, NULL, 1);
    delay_us(7); // t6 delay
    spi_transfer(NULL, buffer, 3);
    CS_1();

    read_val = ((uint32_t)buffer[0] << 16) | ((uint32_t)buffer[1] << 8) | buffer[2];
    if (read_val & 0x800000) { // Sign-extend 24-bit to 32-bit
        read_val |= 0xFF000000;
    }
    return (int32_t)read_val;
}

//####################################################################################
//
//##    5. PUBLIC FUNCTION IMPLEMENTATIONS
//
//####################################################################################

int ads1256_init(void) {
    // --- GPIO Initialization ---
    gpio_chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!gpio_chip) {
        perror("GPIO: Failed to open chip");
        return -1;
    }
    rst_line = gpiod_chip_get_line(gpio_chip, RST_PIN);
    cs_line = gpiod_chip_get_line(gpio_chip, CS_PIN);
    drdy_line = gpiod_chip_get_line(gpio_chip, DRDY_PIN);
    if (!rst_line || !cs_line || !drdy_line) {
        perror("GPIO: Failed to get lines");
        ads1256_exit();
        return -1;
    }
    // Request CS and RST as outputs, DRDY as input with pull-up
    if (gpiod_line_request_output(rst_line, "ads1256_rst", 1) < 0 ||
        gpiod_line_request_output(cs_line, "ads1256_cs", 1) < 0 ||
        gpiod_line_request_input_flags(drdy_line, "ads1256_drdy", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        perror("GPIO: Failed to request lines");
        ads1256_exit();
        return -1;
    }

    // --- SPI Initialization ---
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("SPI: Failed to open device");
        ads1256_exit();
        return -1;
    }
    uint8_t mode = SPI_MODE_1;
    uint8_t lsb_first = 0; // MSB first
    uint32_t speed = 1920000; // ~1.92 MHz (f_CLKIN is 7.68MHz, SPI clock <= f_CLKIN/4)
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &lsb_first) == -1 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("SPI: Failed to configure");
        ads1256_exit();
        return -1;
    }
    
    // --- ADC Reset and Setup ---
    RST_1();
    CS_1();
    write_cmd(CMD_RESET);
    delay_us(10000); // Wait for reset to complete
    wait_for_drdy();

    return 1;
}

void ads1256_exit(void) {
    if (spi_fd >= 0) close(spi_fd);
    if (rst_line) gpiod_line_release(rst_line);
    if (cs_line) gpiod_line_release(cs_line);
    if (drdy_line) gpiod_line_release(drdy_line);
    if (gpio_chip) gpiod_chip_close(gpio_chip);
}

void ads1256_set_pga(ads1256_pga_t gain) {
    uint8_t adcon = read_reg(REG_ADCON);
    adcon = (adcon & 0xF8) | gain; // Clear bottom 3 bits and set gain
    write_reg(REG_ADCON, adcon);
}

void ads1256_set_drate(ads1256_drate_t drate) {
    write_reg(REG_DRATE, drate);
}

void ads1256_set_buffer(adc_bool_t enable) {
    uint8_t status = read_reg(REG_STATUS);
    if (enable) {
        write_reg(REG_STATUS, status | (1 << 1));
    } else {
        write_reg(REG_STATUS, status & ~(1 << 1));
    }
}

void ads1256_calibrate(void) {
    wait_for_drdy();
    write_cmd(CMD_SELFCAL);
    wait_for_drdy(); // Wait for calibration to finish
}

int32_t ads1256_read_single_ended(ads1256_ain_t channel) {
    wait_for_drdy();
    write_reg(REG_MUX, (channel << 4) | AINCOM);
    write_cmd(CMD_SYNC);
    delay_us(5);
    write_cmd(CMD_WAKEUP);
    delay_us(1);
    wait_for_drdy();
    return read_data_raw();
}

int32_t ads1256_read_differential(ads1256_ain_t pos_ch, ads1256_ain_t neg_ch) {
    wait_for_drdy();
    write_reg(REG_MUX, (pos_ch << 4) | neg_ch);
    write_cmd(CMD_SYNC);
    delay_us(5);
    write_cmd(CMD_WAKEUP);
    delay_us(1);
    wait_for_drdy();
    return read_data_raw();
}

void ads1256_start_continuous_se(ads1256_ain_t channel) {
    wait_for_drdy();
    write_reg(REG_MUX, (channel << 4) | AINCOM);
    write_cmd(CMD_SYNC);
    delay_us(5);
    write_cmd(CMD_WAKEUP);
    delay_us(1);
    CS_0();
    wait_for_drdy();
    uint8_t cmd = CMD_RDATAC;
    spi_transfer(&cmd, NULL, 1);
    delay_us(7);
}

void ads1256_stop_continuous(void) {
    wait_for_drdy();
    uint8_t cmd = CMD_SDATAC;
    spi_transfer(&cmd, NULL, 1);
    CS_1();
}

int32_t ads1256_read_continuous_data(void) {
    uint8_t buffer[3];
    uint32_t read_val = 0;

    wait_for_drdy();
    spi_transfer(NULL, buffer, 3);

    read_val = ((uint32_t)buffer[0] << 16) | ((uint32_t)buffer[1] << 8) | buffer[2];
    if (read_val & 0x800000) { // Sign-extend
        read_val |= 0xFF000000;
    }
    return (int32_t)read_val;
}

// --- Scan Mode Function Implementations ---

void ads1256_configure_scan(ads1256_ain_t p_channels[], uint8_t num_channels_to_scan) {
    if (num_channels_to_scan == 0 || num_channels_to_scan > MAX_SCAN_CHANNELS) {
        fprintf(stderr, "Scan: Invalid number of channels (%d). Max is %d.\n", num_channels_to_scan, MAX_SCAN_CHANNELS);
        num_configured_scan_channels = 0;
        return;
    }
    for (uint8_t i = 0; i < num_channels_to_scan; i++) {
        scan_channels[i] = p_channels[i];
    }
    num_configured_scan_channels = num_channels_to_scan;
    current_scan_channel_index = 0;
    // It's good practice to set the MUX to the first channel in the scan here
    // to prepare for the first call to ads1256_read_next_scanned_channel.
    // This avoids reading an undefined channel on the first call if the MUX was not set prior.
    if (num_configured_scan_channels > 0) {
        wait_for_drdy(); // Ensure ADC is ready for commands
        write_reg(REG_MUX, (scan_channels[0] << 4) | AINCOM); // Set MUX to first channel vs AINCOM
        // No SYNC/WAKEUP here, let read_next_scanned_channel handle it to ensure proper timing.
    }
}

int32_t ads1256_read_next_scanned_channel(void) {
    if (num_configured_scan_channels == 0) {
        fprintf(stderr, "Scan: Scan not configured. Call ads1256_configure_scan() first.\n");
        return 0; // Or some error code
    }

    ads1256_ain_t current_channel = scan_channels[current_scan_channel_index];

    // Set MUX for the current channel in the scan sequence
    // This is done even if it's the first channel, as configure_scan only preps it.
    // For subsequent channels, this is where the MUX is updated.
    wait_for_drdy();
    write_reg(REG_MUX, (current_channel << 4) | AINCOM); // PSEL = current_channel, NSEL = AINCOM

    // Issue SYNC and WAKEUP to start conversion on the newly selected channel
    // The ADS1256 datasheet indicates SYNC is needed to make MUX changes effective for the next conversion.
    write_cmd(CMD_SYNC);
    delay_us(5); // t11 delay from datasheet (SYNC to WAKEUP minimum is 4 * t_CLKIN, ~0.52us for 7.68MHz)
                 // Using a slightly longer delay for safety.
    write_cmd(CMD_WAKEUP);
    delay_us(1); // t_SCCS delay (WAKEUP to DRDY low minimum is 24 * t_CLKIN, ~3.125us)
                 // The wait_for_drdy() below will handle the actual wait.

    // Wait for data to be ready and read it
    wait_for_drdy();
    int32_t data = read_data_raw();

    // Advance to the next channel for the next call
    current_scan_channel_index = (current_scan_channel_index + 1) % num_configured_scan_channels;

    return data;
}

void ads1256_end_scan(void) {
    num_configured_scan_channels = 0;
    current_scan_channel_index = 0;
    // Optionally, could send SDATAC if RDATAC was used, but this implementation
    // uses SYNC/WAKEUP per read, so SDATAC is not strictly necessary here unless
    // a continuous read command was somehow active from other operations.
}