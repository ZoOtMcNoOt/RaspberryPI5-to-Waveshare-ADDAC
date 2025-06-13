#include <stdio.h>
#include <stdlib.h>
#include "ads1256.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number_of_samples>\n", argv[0]);
        return 1;
    }
    long num_samples = atol(argv[1]);

    printf("Initializing ADS1256...\n");
    if (ads1256_init() < 0) {
        fprintf(stderr, "Failed to initialize ADS1256. Exiting.\n");
        return 1;
    }

    // Configure the ADC
    ads1256_set_pga(PGA_GAIN_1);
    ads1256_set_drate(DRATE_1000SPS);
    ads1256_set_buffer(ADC_TRUE);
    ads1256_calibrate();
    printf("ADC Configured and Calibrated.\n\n");

    printf("--- Reading %ld single samples from AIN0 ---\n", num_samples);
    for (int i = 0; i < num_samples; i++) {
        int32_t val = ads1256_read_single_ended(AIN0);
        // Vref = 5V, 24-bit range is 2^23 - 1 for positive values
        double voltage = (double)val / 8388607.0 * 5.0;
        printf("Sample %d: %d\t(Voltage: %f V)\n", i + 1, val, voltage);
    }
    printf("\n");

    ads1256_exit();
    printf("Program finished.\n");

    return 0;
}