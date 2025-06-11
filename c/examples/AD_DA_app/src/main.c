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
    int i;                   

    printf("AD/DA Example Application Started.\r\n");

    if (DEV_ModuleInit() != 0) {
        printf("Hardware Initialization Failed.\r\n");
        return 1;
    }

    signal(SIGINT, SigintHandler);

    if (ADS1256_init(ADS1256_DRATE_30000SPS, ADS1256_GAIN_1, SCAN_MODE_DIFFERENTIAL_INPUTS) != 0) { 
        printf("ADS1256 Initialization Failed.\r\n");
        DEV_ModuleExit();
        return 1;
    }
    printf("ADS1256 Initialized Successfully.\r\n");

    while (1) {
        ADS1256_GetAll(adc_readings);

        printf("ADC Readings:\r\n");
        for (i = 0; i < 8; i++) {
            float voltage = ADS1256_RawToVoltage(adc_readings[i], ADS1256_GAIN_1, ADC_VREF_POS_5V0, ADC_VREF_NEG_GND); 
            printf("  CH%d: 0x%06lX => %.4f V\r\n", i, adc_readings[i], voltage);
        }

        voltage_ch0 = ADS1256_RawToVoltage(adc_readings[0], ADS1256_GAIN_1, ADC_VREF_POS_5V0, ADC_VREF_NEG_GND);
        
        if (voltage_ch0 < 0.0f) voltage_ch0 = 0.0f;
        if (voltage_ch0 > DAC_VREF) voltage_ch0 = DAC_VREF;

        printf("Setting DAC Outputs based on CH0 (%.4f V):\r\n", voltage_ch0);

        DAC8532_Out_Voltage(DAC8532_CHANNEL_B, voltage_ch0);
        printf("  DAC CH_B: %.4f V\r\n", voltage_ch0);

        float voltage_chA = DAC_VREF - voltage_ch0;
        if (voltage_chA < 0.0f) voltage_chA = 0.0f; 
        DAC8532_Out_Voltage(DAC8532_CHANNEL_A, voltage_chA);
        printf("  DAC CH_A: %.4f V\r\n", voltage_chA);

        printf("\033[12A"); 

    }

    DEV_ModuleExit();
    return 0;
}