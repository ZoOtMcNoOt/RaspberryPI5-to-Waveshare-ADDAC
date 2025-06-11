/**
 * @file DAC8532.h
 * @brief Header file for DAC8532 Digital-to-Analog Converter driver.
 *
 * This file contains the definitions, constants, and function prototypes
 * for interfacing with the DAC8532 dual-channel, 16-bit, serial input
 * Digital-to-Analog Converter.
 *
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef _DAC8532_H_
#define _DAC8532_H_

#include "../../common/DEV_Config.h" // Ensure this path is correct

// Define DAC8532 Channel selection constants
// These are typically command bytes sent to the DAC
// Consult the DAC8532 datasheet for the correct values and meanings.
// Example: (These might need adjustment based on datasheet)
//   - Bit 7-6: Don't care (or specific settings)
//   - Bit 5-4: Power-down mode (00 = normal operation)
//   - Bit 3-1: Address bits for DAC channel (e.g., 000 for DAC A, 001 for DAC B)
//   - Bit 0:   Load mode (0 = update DAC register, 1 = update DAC output)
// For simplicity, assuming direct channel selection with specific command bytes.
// The original defines (0x30, 0x34) might be specific to a particular mode.
// Let's make them more descriptive if their function is known, or keep as is if they are opaque command bytes.

/** @brief Command/Address for DAC Channel A. 
 *  This value is sent to select Channel A and set its operating mode. 
 *  (Value 0x30 might imply specific mode settings, check datasheet)
 */
#define DAC8532_CHANNEL_A   0x30 

/** @brief Command/Address for DAC Channel B. 
 *  This value is sent to select Channel B and set its operating mode. 
 *  (Value 0x34 might imply specific mode settings, check datasheet)
 */
#define DAC8532_CHANNEL_B   0x34

/** @brief Maximum digital value for the 16-bit DAC (2^16 - 1). */
#define DAC_VALUE_MAX  65535U

/** @brief Reference voltage for the DAC in Volts. */
#define DAC_VREF  5.0f

/**
 * @brief Sets the output voltage for a specified DAC channel.
 * 
 * Converts a desired floating-point voltage into the corresponding
 * 16-bit digital value and writes it to the DAC.
 * 
 * @param Channel The DAC channel to set the voltage for (e.g., `DAC8532_CHANNEL_A`).
 * @param Voltage The desired output voltage (0.0 to `DAC_VREF`).
 */
void DAC8532_Out_Voltage(UBYTE Channel, float Voltage);

/**
 * @brief Writes a 16-bit data value to the specified DAC channel.
 * 
 * This is a lower-level function to send raw digital data to the DAC.
 * 
 * @param Channel The DAC channel to write to (e.g., `DAC8532_CHANNEL_A`).
 * @param Data The 16-bit data value to write to the DAC.
 */
void Write_DAC8532(UBYTE Channel, UWORD Data);

#endif // _DAC8532_H_