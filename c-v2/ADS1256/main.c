#include <stdio.h>
#include <stdlib.h>
#define _POSIX_C_SOURCE 199309L // Required for clock_gettime
#include <time.h> // Added for time measurement
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
    ads1256_set_drate(DRATE_15000SPS);
    ads1256_set_buffer(ADC_TRUE);
    ads1256_calibrate();
    printf("ADC Configured and Calibrated.\n\n");

    printf("--- Reading %ld single samples from AIN0 ---\n", num_samples);

    struct timespec start_time, end_time; // Variables for time measurement
    clock_gettime(CLOCK_MONOTONIC, &start_time); // Get start time

    for (int i = 0; i < num_samples; i++) {
        int32_t val = ads1256_read_single_ended(AIN0);
        // Vref = 5V, 24-bit range is 2^23 - 1 for positive values
        double voltage = (double)val / 8388607.0 * 5.0;
        printf("Sample %d: %d\t(Voltage: %f V)\n", i + 1, val, voltage); // Re-enabled live reading
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time); // Get end time

    // Calculate elapsed time
    double elapsed_time_s = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("\n");
    if (elapsed_time_s > 0) {
        double sps = num_samples / elapsed_time_s;
        printf("Read %ld samples in %.4f seconds.\n", num_samples, elapsed_time_s);
        printf("Actual Samples Per Second (SPS): %.2f\n", sps);
    } else {
        printf("Elapsed time was too short to calculate SPS accurately.\n");
    }
    printf("\n");

    ads1256_exit();
    printf("Program finished.\n");

    return 0;
}