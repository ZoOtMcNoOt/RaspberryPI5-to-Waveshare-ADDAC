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
#include <stdlib.h>     
#include <signal.h>     
#include <time.h>       
#include <string.h>     
#include <stdio.h>      

#include "../../lib/ADS1256/ADS1256.h"
#include "../../lib/DAC8532/DAC8532.h"
#include "../../common/DEV_Config.h" 
#include "../../common/Debug.h"      

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
    UDOUBLE adc_readings[8]; 
    float voltage_ch0;       

    printf("AD/DA Example Application Started.\r\n");

    if (DEV_ModuleInit() != 0) {
        printf("Hardware Initialization Failed.\r\n");
        return 1;
    }

    signal(SIGINT, SigintHandler);

    if (ADS1256_init(ADS1256_30000SPS, ADS1256_GAIN_1, SCAN_MODE_DIFFERENTIAL_INPUTS) != 0) { 
        printf("ADS1256 Initialization Failed.\r\n");
        DEV_ModuleExit();
        return 1;
    }
    printf("ADS1256 Initialized Successfully.\r\n");

    while (1) {
        ADS1256_GetAllChannels(adc_readings);

        // The rest of the logic uses adc_readings[0] which corresponds to AIN0-AIN1
        voltage_ch0 = ADS1256_RawToVoltage(adc_readings[0], ADS1256_GAIN_1, ADC_VREF_POS_5V0, ADC_VREF_NEG_GND);
        
        if (voltage_ch0 < 0.0f) voltage_ch0 = 0.0f;
        if (voltage_ch0 > DAC_VREF) voltage_ch0 = DAC_VREF; // DAC_VREF is used from the original logic

        DAC8532_Out_Voltage(DAC8532_CHANNEL_B, voltage_ch0);

        float voltage_chA = DAC_VREF - voltage_ch0; // DAC_VREF is used from the original logic
        if (voltage_chA < 0.0f) voltage_chA = 0.0f; 
        DAC8532_Out_Voltage(DAC8532_CHANNEL_A, voltage_chA);

        // Print concise output on a single line, overwriting the previous line.
        // Pad with spaces to clear any remnants of a longer previous line.
        printf("\rADC CH0: %.4f V | DAC A: %.4f V | DAC B: %.4f V                ", voltage_ch0, voltage_chA, voltage_ch0);
        fflush(stdout); // Ensure output is displayed immediately
    }

    DEV_ModuleExit();
    return 0;
}