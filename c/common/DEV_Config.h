/**
 * @file DEV_Config.h
 * @brief Hardware configuration and interface definitions for AD/DA module.
 *
 * This header file defines constants, macros, and function prototypes for
 * interfacing with an AD/DA (Analog-to-Digital / Digital-to-Analog) module.
 * It includes GPIO pin definitions, SPI communication macros, and type
 * definitions for common data sizes.
 * It is intended for use with a Raspberry Pi, utilizing the gpiod library for GPIO control
 * and spidev for SPI communication.
 */
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h> // For uint8_t, uint16_t, uint32_t
#include <stdio.h>  // For perror, fprintf
#include <gpiod.h>  // For gpiod structures and functions
#include "Debug.h"   // For Debug() macro

// Typedefs for common data sizes
#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

/** @name SPI Device Configuration */
#define SPI_DEVICE      "/dev/spidev0.0" ///< SPI device path
#define SPI_SPEED_HZ    1800000          ///< SPI clock speed in Hz (1.8 MHz)

/** @name GPIO Chip Configuration */
#define GPIO_CHIP_NAME  "gpiochip4"      ///< GPIO chip name (e.g., for Raspberry Pi 5)

/** @name GPIO Pin Definitions (BCM numbering) */
#define DEV_RST_PIN     18  ///< ADC Reset pin (GPIO18)
#define DEV_CS_PIN      22  ///< ADC Chip Select pin (GPIO22)
#define DEV_CS1_PIN     23  ///< DAC Chip Select pin (GPIO23)
#define DEV_DRDY_PIN    17  ///< ADC Data Ready pin (GPIO17)

/** @name GPIO Pin State Definitions */
#define HIGH            1   ///< GPIO pin high state
#define LOW             0   ///< GPIO pin low state

/** @name GPIO Access Macros */
#define DEV_Digital_Write(_pin, _value) DEV_GPIO_Write(_pin, _value) ///< Macro to write to a GPIO pin
#define DEV_Digital_Read(_pin)          DEV_GPIO_Read(_pin)          ///< Macro to read from a GPIO pin

/** @name SPI Communication Macros */
#define DEV_SPI_WriteByte(_dat) SPI_WriteByte(_dat) ///< Macro to write a byte via SPI
#define DEV_SPI_ReadByte()      SPI_ReadByte()      ///< Macro to read a byte via SPI

/** @name Delay Macro */
#define DEV_Delay_ms(__xms) DEV_Delay_ms_func(__xms) ///< Macro for millisecond delay

/*---------------------------------------------------------------------------
                            Function Prototypes
---------------------------------------------------------------------------*/

/**
 * @brief Initializes the SPI device and GPIO lines.
 * @return 0 on success, 1 on failure.
 */
int DEV_ModuleInit(void);

/**
 * @brief Releases SPI and GPIO resources.
 */
void DEV_ModuleExit(void);

/**
 * @brief Writes a byte to the SPI bus.
 * @param value The byte to write.
 */
void SPI_WriteByte(uint8_t value);

/**
 * @brief Reads a byte from the SPI bus.
 * @return The byte read from SPI.
 */
UBYTE SPI_ReadByte(void);

/**
 * @brief Writes a digital value to a specified GPIO pin.
 * @param pin The GPIO pin number (e.g., DEV_RST_PIN).
 * @param value The value to write (0 or 1).
 */
void DEV_GPIO_Write(int pin, int value);

/**
 * @brief Reads the digital value from a specified GPIO pin.
 * @param pin The GPIO pin number (e.g., DEV_DRDY_PIN).
 * @return The value of the pin (0 or 1), or -1 on error.
 */
int DEV_GPIO_Read(int pin);

/**
 * @brief Delays execution for a specified number of milliseconds.
 * @param ms The delay time in milliseconds.
 */
void DEV_Delay_ms_func(int ms);

#endif // _DEV_CONFIG_H_