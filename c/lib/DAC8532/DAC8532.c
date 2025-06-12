/**
 * @file DAC8532.c
 * @brief Implementation for DAC8532 Digital-to-Analog Converter driver.
 *
 * This file contains the function definitions for interfacing with the DAC8532
 * dual-channel, 16-bit, serial input Digital-to-Analog Converter.
 *
 * @copyright Copyright (c) 2023
 * 
 */
#include "DAC8532.h"
#include "../../common/DEV_Config.h"

/**
 * @brief Writes a 16-bit data value to the specified DAC channel.
 * 
 * This function sends a 24-bit serial word to the DAC8532.
 * The first byte specifies the channel and operation mode.
 * The next two bytes are the 16-bit data for the DAC.
 * 
 * @param Channel The DAC channel to write to.
 *                Use `DAC8532_CHANNEL_A` or `DAC8532_CHANNEL_B`.
 * @param Data The 16-bit data value to write to the DAC.
 */
void Write_DAC8532(UBYTE Channel, UWORD Data)
{
    DEV_Digital_Write(DEV_CS1_PIN, LOW); 

    DEV_SPI_WriteByte(Channel);
    DEV_SPI_WriteByte((Data >> 8) & 0xFF); 
    DEV_SPI_WriteByte(Data & 0xFF);      

    DEV_Digital_Write(DEV_CS1_PIN, HIGH); 
}

/**
 * @brief Sets the output voltage for a specified DAC channel.
 * 
 * This function converts a desired floating-point voltage into the corresponding
 * 16-bit digital value and writes it to the DAC. The voltage must be within
 * the DAC's reference voltage range (0 to V_REF).
 * 
 * @param Channel The DAC channel to set the voltage for.
 *                Use `DAC8532_CHANNEL_A` or `DAC8532_CHANNEL_B`.
 * @param Voltage The desired output voltage (0.0 to V_REF).
 */
void DAC8532_Out_Voltage(UBYTE Channel, float Voltage)
{
    UWORD digital_value = 0;

    if (Voltage > DAC_VREF) {
        Voltage = DAC_VREF;
    } else if (Voltage < 0.0f) {
        Voltage = 0.0f;
    }

    digital_value = (UWORD)((Voltage / DAC_VREF) * DAC_VALUE_MAX);

    Write_DAC8532(Channel, digital_value);
}