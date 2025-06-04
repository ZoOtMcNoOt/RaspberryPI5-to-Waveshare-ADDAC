
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_


#include <stdint.h>
#include <stdio.h>
#include "Debug.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>




#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIO config
**/
#define DEV_RST_PIN     18
#define DEV_CS_PIN      22
#define DEV_DRDY_PIN    17

/**
 * GPIO read and write
**/ 
#define DEV_Digital_Write(_pin, _value)  digitalWrite(_pin, _value == 0? LOW:HIGH)
#define DEV_Digital_Read(_pin)  digitalRead(_pin)


/**
 * SPI
**/
#define DEV_SPI_WriteByte(_dat)  SPI_WriteByte(_dat)
#define DEV_SPI_ReadByte()  SPI_ReadByte()

/**
 * delay x ms
**/
#define DEV_Delay_ms(__xms)   delay(__xms)


/*-----------------------------------------------------------------------------*/
void SPI_WriteByte(uint8_t value);
UBYTE SPI_ReadByte(void);
int DEV_ModuleInit(void);
void DEV_ModuleExit(void);
#endif
