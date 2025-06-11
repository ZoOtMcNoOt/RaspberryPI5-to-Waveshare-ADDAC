/**
 * @file main.c
 * @brief Example application demonstrating ADC (ADS1256) and DAC (DAC8532) functionality.
 *
 * This application initializes the ADS1256 ADC and DAC8532 DAC, continuously reads
 * all 8 ADC channels, prints their values, and then uses the value from ADC channel 0
 * to set the output voltages on the two DAC channels.
 * - DAC Channel B is set to the scaled voltage from ADC Channel 0.
 * - DAC Channel A is set to (V_REF - scaled voltage from ADC Channel 0).
 *
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdlib.h>     // For exit()
#include <signal.h>     // For signal()
#include <time.h>       // For time-related functions (not explicitly used but good for context)
#include <string.h>     // For string functions (not explicitly used but good for context)
#include <stdio.h>      // For printf()

#include "../../lib/ADS1256/ADS1256.h"
#include "../../lib/DAC8532/DAC8532.h"
#include "../../common/DEV_Config.h" // For DEV_ModuleInit, DEV_ModuleExit
#include "../../common/Debug.h"      // For Debug messages (if any)

/**
 * @brief Signal handler for graceful termination (e.g., Ctrl+C).
 *
 * This function is called when an interrupt signal (SIGINT) is received.
 * It prints an exit message, cleans up resources by calling DEV_ModuleExit(),
 * and then terminates the program.
 *
 * @param signo The signal number that triggered the handler (unused in this function).
 */
void SigintHandler(int signo)
{
    printf("\r\nExiting program.\r\n");
    DEV_ModuleExit();
    exit(0);
}

/**
 * @brief Main function for the AD/DA application.
 *
 * Initializes the hardware modules (ADC and DAC), then enters an infinite loop
 * to continuously read ADC values and set DAC outputs.
 *
 * @return int Program exit status (0 for success, non-zero for failure).
 */
int main(void)
{
    UDOUBLE adc_readings[8]; // Array to store readings from 8 ADC channels
    float voltage_ch0;       // Variable to store the calculated voltage from ADC channel 0
    int i;                   // Loop counter

    printf("AD/DA Example Application Started.\r\n");

    // Initialize the development board modules
    if (DEV_ModuleInit() != 0) {
        printf("Hardware Initialization Failed.\r\n");
        return 1;
    }

    // Register the signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, SigintHandler);

    // Initialize the ADS1256 ADC
    // The ADS1256_init function likely performs calibration and setup.
    // It might return a status code (e.g., 0 for success, 1 for failure).
    if (ADS1256_init(ADS1256_DRATE_30000SPS, ADS1256_GAIN_1, SCAN_MODE_DIFFERENTIAL_INPUTS) != 0) { // Assuming SCAN_MODE_DIFFERENTIAL_INPUTS is a valid mode
        printf("ADS1256 Initialization Failed.\r\n");
        DEV_ModuleExit();
        return 1;
    }
    printf("ADS1256 Initialized Successfully.\r\n");

    // Main application loop
    while (1) {
        // Read all 8 ADC channels
        // ADS1256_GetAll should populate the adc_readings array.
        // This function might block until all readings are available.
        ADS1256_GetAll(adc_readings);

        printf("ADC Readings:\r\n");
        for (i = 0; i < 8; i++) {
            // Convert raw ADC reading to voltage.
            // The formula V = (RawADC * V_REF) / (2^23 -1) is common for a 24-bit ADC.
            // ADS1256_RawToVoltage should encapsulate this conversion.
            float voltage = ADS1256_RawToVoltage(adc_readings[i], ADS1256_GAIN_1, ADC_VREF_POS_5V0, ADC_VREF_NEG_GND); // Assuming 5V Vref and Gain 1
            printf("  CH%d: 0x%06lX => %.4f V\r\n", i, adc_readings[i], voltage);
        }

        // Calculate voltage for DAC output based on ADC channel 0
        // The original calculation: voltage_ch0 = (adc_readings[0] >> 7) * 5.0 / 0xffff;
        // This seems to be a specific scaling, possibly for a 16-bit DAC or a specific range.
        // Let's use the proper conversion for ADC CH0 first.
        voltage_ch0 = ADS1256_RawToVoltage(adc_readings[0], ADS1256_GAIN_1, ADC_VREF_POS_5V0, ADC_VREF_NEG_GND);
        
        // Ensure voltage_ch0 is within DAC limits (0-5V for DAC8532 with 5V Vref)
        if (voltage_ch0 < 0.0f) voltage_ch0 = 0.0f;
        if (voltage_ch0 > DAC_VREF) voltage_ch0 = DAC_VREF;

        printf("Setting DAC Outputs based on CH0 (%.4f V):\r\n", voltage_ch0);

        // Set DAC Channel B to the voltage from ADC Channel 0
        DAC8532_Out_Voltage(DAC8532_CHANNEL_B, voltage_ch0);
        printf("  DAC CH_B: %.4f V\r\n", voltage_ch0);

        // Set DAC Channel A to (V_REF - voltage_from_ADC_CH0)
        float voltage_chA = DAC_VREF - voltage_ch0;
        if (voltage_chA < 0.0f) voltage_chA = 0.0f; // Clamp to ensure non-negative
        DAC8532_Out_Voltage(DAC8532_CHANNEL_A, voltage_chA);
        printf("  DAC CH_A: %.4f V\r\n", voltage_chA);

        // ANSI escape code to move cursor up 11 lines (8 ADC + 1 ADC title + 2 DAC + 1 DAC title)
        // Adjust this based on the actual number of lines printed in each loop iteration.
        printf("\033[12A"); 

        // Add a small delay if needed, e.g., DEV_Delay_ms(100);
    }

    // Cleanup (though unreachable in this infinite loop, good practice for other scenarios)
    DEV_ModuleExit();
    return 0;
}