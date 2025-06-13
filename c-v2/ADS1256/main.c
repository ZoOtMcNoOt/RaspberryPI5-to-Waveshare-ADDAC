#include <stdio.h>
#include <stdlib.h>
#define _POSIX_C_SOURCE 199309L // Required for clock_gettime
#include <time.h> // Added for time measurement
#include "ads1256.h"

#define CSV_FILENAME "adc_scan_data.csv"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number_of_scan_cycles>\n", argv[0]);
        return 1;
    }
    long num_scan_cycles = atol(argv[1]);

    FILE *csv_file = fopen(CSV_FILENAME, "w");
    if (csv_file == NULL) {
        perror("Error opening CSV file");
        return 1;
    }

    printf("Initializing ADS1256...\n");
    if (ads1256_init() < 0) {
        fprintf(stderr, "Failed to initialize ADS1256. Exiting.\n");
        return 1;
    }

    // Configure the ADC
    ads1256_set_pga(PGA_GAIN_1);
    ads1256_set_drate(DRATE_30000SPS); // Set to desired data rate
    ads1256_set_buffer(ADC_TRUE);
    ads1256_calibrate();
    printf("ADC Configured and Calibrated.\n\n");

    // Define channels for scanning
    ads1256_ain_t channels_to_scan[] = {AIN0, AIN1, AIN2, AIN3};
    uint8_t num_channels = sizeof(channels_to_scan) / sizeof(channels_to_scan[0]);

    // Write CSV header
    fprintf(csv_file, "SampleSet");
    for (uint8_t i = 0; i < num_channels; i++) {
        fprintf(csv_file, ",AIN%d", channels_to_scan[i]);
    }
    fprintf(csv_file, "\n");

    // Configure the scan sequence in the driver
    ads1256_configure_scan(channels_to_scan, num_channels);

    printf("--- Reading %ld scan cycles from %d channels ---\n", num_scan_cycles, num_channels);

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time); // Get start time

    for (long cycle = 0; cycle < num_scan_cycles; cycle++) {
        fprintf(csv_file, "%ld", cycle + 1);
        // printf("Scan Cycle %ld:\n", cycle + 1); // Optional: for debugging, slows down significantly
        for (uint8_t ch_idx = 0; ch_idx < num_channels; ch_idx++) {
            int32_t val = ads1256_read_next_scanned_channel();
            fprintf(csv_file, ",%d", val);
            // double voltage = (double)val / 8388607.0 * 5.0; // Vref=5V, 2^23-1 for positive range
            // printf("  AIN%d: %d\t(Voltage: %f V)\n", scan_channels[ch_idx], val, voltage); // Optional
        }
        fprintf(csv_file, "\n");
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time); // Get end time
    ads1256_end_scan(); // Clean up scan configuration
    fclose(csv_file); // Close the CSV file

    // Calculate elapsed time
    double elapsed_time_s = (end_time.tv_sec - start_time.tv_sec) +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    long total_samples_read = num_scan_cycles * num_channels;

    printf("\n");
    if (elapsed_time_s > 0) {
        double aggregate_sps = total_samples_read / elapsed_time_s;
        printf("Read %ld total samples (%ld cycles of %d channels) in %.4f seconds.\n",
               total_samples_read, num_scan_cycles, num_channels, elapsed_time_s);
        printf("Actual Aggregate Samples Per Second (SPS): %.2f\n", aggregate_sps);
        if (num_channels > 0) {
            printf("Per-Channel Samples Per Second (SPS) during scan: %.2f\n", aggregate_sps / num_channels);
        }
    } else {
        printf("Elapsed time was too short to calculate SPS accurately.\n");
    }
    printf("\n");

    ads1256_exit();
    printf("Data written to %s\n", CSV_FILENAME);
    printf("Program finished.\n");

    return 0;
}