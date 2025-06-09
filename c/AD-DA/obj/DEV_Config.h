#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <gpiod.h>
#include "Debug.h"

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/**
 * GPIO config - All pins for both ADC and DAC
 * Note: The pin numbers are based on the GPIO numbering scheme I thought they
 * the physical pin layout of the Raspberry Pi but I was wrong.
 */

#define DEV_RST_PIN     18  //ADC Reset
#define DEV_CS_PIN      22  //ADC Chip Select
#define DEV_CS1_PIN     23  //DAC Chip Select
#define DEV_DRDY_PIN    17  //ADC Data Ready

/**
 * GPIO read and write
 */

#define DEV_Digital_Write(_pin, _value) DEV_GPIO_Write(_pin, _value)
#define DEV_Digital_Read(_pin) DEV_GPIO_Read(_pin)

/**
 * SPI
 */
#define DEV_SPI_WriteByte(_dat) SPI_WriteByte(_dat)
#define DEV_SPI_ReadByte() SPI_ReadByte()

/**
 * delay x ms
 */
#define DEV_Delay_ms(__xms) DEV_Delay_ms_func(__xms)

/*---------------------------------------------------------------------------*/
void SPI_WriteByte(uint8_t value);
UBYTE SPI_ReadByte(void);
int DEV_ModuleInit(void);
void DEV_ModuleExit(void);
void DEV_GPIO_Write(int pin, int value);
int DEV_GPIO_Read(int pin);
void DEV_Delay_ms_func(int ms);
 
#endif