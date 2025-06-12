/*
	Author: Jure Bartol (Original), Modified for libgpiod/RPi5 support
	Date: 07.05.2016 (Original), Modified: 2025
	
	Modified to use libgpiod instead of bcm2835 for Raspberry Pi 5 support
*/

#ifndef ADS1256_H_INCLUDED
#define ADS1256_H_INCLUDED

#include <stdio.h>
#include <stdint.h>

// Boolean values.
typedef enum
{
	False = 0,
	True  = 1,
} bool;

/*	*******************************
	** PART 1 - serial interface **
	*******************************
	Functions:
		- delayus()
		- send8bit()
		- recieve8bit()
		- waitDRDY()
		- initializeSPI()
		- endSPI()
*/

void    delayus(uint64_t microseconds);
void    send8bit(uint8_t data);
uint8_t recieve8bit(void);
void    waitDRDY(void);
uint8_t initializeSPI();
void    endSPI();

/*	*****************************
	** PART 2 - ads1256 driver **
	*****************************
	Functions:
		- readByteFromReg()
		- writeByteToReg()
		- writeCMD()
		- setBuffer()
		- readChipID()
		- setSEChannel()
		- setDIFFChannel()
		- setPGA()
		- setDataRate()
		- readData()
		- getValSEChannel()
		- getValDIFFChannel()
		- scanSEChannels()
		- scanDIFFChannels()
		- scanSEChannelsContinuous()
		- scanDIFFChannelsContinuous()
*/

uint8_t readByteFromReg(uint8_t registerID);
void    writeByteToReg(uint8_t registerID, uint8_t value);
uint8_t writeCMD(uint8_t command);
uint8_t setBuffer(bool val);
uint8_t readChipID(void);
void    setSEChannel(uint8_t channel);
void    setDIFFChannel(uint8_t positiveCh, uint8_t negativeCh);
void    setPGA(uint8_t pga);
void    setDataRate(uint8_t drate);
int32_t readData(void);
int32_t getValSEChannel(uint8_t channel);
int32_t getValDIFFChannel(uint8_t positiveCh, uint8_t negativeCh);
void    scanSEChannels(uint8_t channels[], uint8_t numOfChannels, uint32_t *values);
void    scanDIFFChannels(uint8_t positiveChs[], uint8_t negativeChs[], uint8_t numOfChannels, uint32_t *values);
void    scanSEChannelContinuous(uint8_t channel, uint32_t numOfMeasure, uint32_t *values, uint32_t *currentTime);
void    scanDIFFChannelContinuous(uint8_t positiveCh, uint8_t negativeCh, uint32_t numOfMeasure, uint32_t *values, uint32_t *currentTime);

// PGA gain settings
enum {
	PGA_GAIN1	= 0,
	PGA_GAIN2	= 1,
	PGA_GAIN4	= 2,
	PGA_GAIN8	= 3,
	PGA_GAIN16	= 4,
	PGA_GAIN32	= 5,
	PGA_GAIN64	= 6
};

// Data rate settings
enum {
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

// Input channels
enum {
	AIN0   = 0,
	AIN1   = 1,
	AIN2   = 2,
	AIN3   = 3,
	AIN4   = 4,
	AIN5   = 5,
	AIN6   = 6,
	AIN7   = 7,
	AINCOM = 8
};

#endif