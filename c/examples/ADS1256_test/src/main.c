#define _DEFAULT_SOURCE // Changed from _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h> // For usleep
#include <math.h>   // For fabs or other math functions if needed
#include "../../lib/ADS1256/ADS1256.h"
#include "../../common/Debug.h" 
#include <stdio.h> // Changed from "stdio.h"

// Forward declarations
const char* drate_to_string(ADS1256_DRATE drate);
const char* gain_to_string(ADS1256_GAIN gain);

// Global control variables
static volatile int running = 1;
static int display_mode = 1; // 1=continuous, 2=summary only
static ADS1256_DRATE drate_setting = ADS1256_30000SPS; // Default DRATE
static ADS1256_GAIN gain_setting = ADS1256_GAIN_1;     // Default GAIN
static UBYTE selected_channels[NUM_SINGLE_ENDED_CHANNELS] = {0, 1, 2, 3}; // Default channels
static int num_selected_channels = 4; // Default number of channels
static int adc_reinit_required = 0; // Flag to indicate if ADC needs re-initialization

void Handler(int signo) {
    running = 0;
    printf("\r\n=== Shutting down gracefully ===\r\n\n");
}

void print_usage_info(void) {
    printf("\n=== ADS1256 N-Channel Optimized Performance Test ===\n");
    printf("This program demonstrates optimized N-channel ADC sampling with:\n");
    printf("- Proper settling time handling\n");
    printf("- Real-time performance metrics vs theoretical limits\n");
    printf("- Efficiency monitoring and optimization feedback\n");
    printf("- Configurable DRATE, GAIN, and Channels\n\n");
}

void print_channel_config(UBYTE *channels, int num_ch) {
    printf("\n--- Channel Configuration ---\n");
    for(int i = 0; i < num_ch; i++) {
        printf("  Slot %d -> AIN%d\n", i, channels[i]);
    }
    printf("All channels: Single-ended, referenced to AINCOM\n");
    // Assuming gain 1 and 5V reference for this example text
    printf("Voltage reference: %.2fV to %.2fV\n", ADC_VREF_NEG_GND, ADC_VREF_POS_5V0);
    printf("Current DRATE: %s, Current GAIN: %s\n\n", drate_to_string(drate_setting), gain_to_string(gain_setting));
}

void test_n_channel_optimized(UBYTE *channels, int num_ch, ADS1256_DRATE current_drate, ADS1256_GAIN current_gain) {
    printf("\n=== Testing %d-Channel Optimized (Full Settling Time) ===\n", num_ch);
    
    UDOUBLE *ADC = malloc(num_ch * sizeof(UDOUBLE));
    if (!ADC) {
        perror("Failed to allocate memory for ADC readings");
        return;
    }
    performance_metrics_t *metrics;
    int sample_count = 0;
    time_t last_report_time = time(NULL);
    const UBYTE settling_cycles = 5; // Example settling cycles
    
    ADS1256_InitPerformanceMonitoring(current_drate);
    metrics = ADS1256_GetPerformanceMetrics(); // Get initial metrics pointer
    
    printf("Starting optimized %d-channel sampling...\n", num_ch);
    printf("Theoretical SPS per channel: %.0f\n", metrics->theoretical_sps_per_channel);
    printf("Press Ctrl+C to stop and view final report\n\n");
    
    if(display_mode == 1) {
        for(int i=0; i<num_ch; ++i) printf("Ch%d(V)\t\t", channels[i]);
        printf("Rate/Ch\t\tEff%%\t\tStatus\n");
        for(int i=0; i<num_ch; ++i) printf("--------\t\t");
        printf("--------\t\t-----\t\t------\n");
    }
    
    while(running) {
        ADS1256_GetNChannels_Optimized(ADC, channels, num_ch, settling_cycles);
        sample_count++; // This counts N-channel scans
        // metrics are updated internally by GetNChannels_Optimized when monitoring is active
        
        if(display_mode == 1) {
            if(sample_count % 10 == 0) { // Update display less frequently
                for(int i = 0; i < num_ch; i++) {
                    float voltage = ADS1256_RawToVoltage(ADC[i], ADC_VREF_POS_5V0, ADC_VREF_NEG_GND, current_gain);
                    printf("%.4f\t\t", voltage);
                }
                printf("%.1f\t\t%.1f\t\t", 
                       metrics->actual_avg_sps_per_channel, metrics->efficiency_percent);
                
                if(metrics->efficiency_percent > 85) printf("EXCELLENT\r");
                else if(metrics->efficiency_percent > 70) printf("GOOD\r");
                else if(metrics->efficiency_percent > 50) printf("FAIR\r");
                else printf("POOR\r");
                fflush(stdout);
            }
        }
        
        time_t current_time_val = time(NULL);
        if(current_time_val - last_report_time >= 5) {
            if(display_mode == 2 || display_mode == 1) { // Also print summary if in continuous mode
                printf("\n--- %d-Second Update ---\n", (int)(current_time_val - last_report_time));
                ADS1256_PrintPerformanceReport(); // Use the library\'s print function
                printf("\n"); // Add a newline after the report
            }
            last_report_time = current_time_val;
        }
        usleep(100); // Small delay
    }
    
    printf("\n\n");
    ADS1256_PrintPerformanceReport();
    free(ADC);
}

void test_n_channel_fast(UBYTE *channels, int num_ch, ADS1256_DRATE current_drate, ADS1256_GAIN current_gain) {
    printf("\n=== Testing %d-Channel Fast (Reduced Settling Time) ===\n", num_ch);
    
    UDOUBLE *ADC = malloc(num_ch * sizeof(UDOUBLE));
     if (!ADC) {
        perror("Failed to allocate memory for ADC readings");
        return;
    }
    struct timeval start_tv, current_tv;
    unsigned long total_scans_fast = 0;
    
    // Re-initialize monitoring for this specific test for accurate theoretical SPS
    ADS1256_InitPerformanceMonitoring(current_drate);
    performance_metrics_t *metrics = ADS1256_GetPerformanceMetrics();

    gettimeofday(&start_tv, NULL);
    
    printf("Starting fast %d-channel sampling...\n", num_ch);
    printf("Theoretical SPS per channel (for DRATE): %.0f\n", metrics->theoretical_sps_per_channel);
    printf("Press Ctrl+C to stop\n\n");

    for(int i=0; i<num_ch; ++i) printf("Ch%d(V)\t\t", channels[i]);
    printf("Rate/Ch (Actual)\n");
    for(int i=0; i<num_ch; ++i) printf("--------\t\t");
    printf("----------------\n");
    
    while(running) {
        ADS1256_GetNChannels_Fast(ADC, channels, num_ch);
        total_scans_fast++;
        
        if(total_scans_fast % 20 == 0) { // Update display less frequently
            gettimeofday(&current_tv, NULL);
            double elapsed_sec = (current_tv.tv_sec - start_tv.tv_sec) + 
                                 (current_tv.tv_usec - start_tv.tv_usec) / 1000000.0;
            double rate_per_channel_actual = (elapsed_sec > 0) ? (total_scans_fast / elapsed_sec) : 0;
            
            for(int i = 0; i < num_ch; i++) {
                float voltage = ADS1256_RawToVoltage(ADC[i], ADC_VREF_POS_5V0, ADC_VREF_NEG_GND, current_gain);
                printf("%.4f\t\t", voltage);
            }
            printf("%.1f\r", rate_per_channel_actual);
            fflush(stdout);
        }
        usleep(50); // Small delay
    }
    
    gettimeofday(&current_tv, NULL);
    double total_elapsed_sec = (current_tv.tv_sec - start_tv.tv_sec) + 
                              (current_tv.tv_usec - start_tv.tv_usec) / 1000000.0;
    double final_rate_actual = (total_elapsed_sec > 0) ? (total_scans_fast / total_elapsed_sec) : 0;

    printf("\n\n=== Fast Mode Results ===\n");
    printf("Total %d-channel scans: %lu\n", num_ch, total_scans_fast);
    printf("Runtime: %.2f seconds\n", total_elapsed_sec);
    printf("Actual Rate per channel: %.1f SPS\n", final_rate_actual);
    printf("Actual Total rate for %d channels: %.1f SPS\n", num_ch, final_rate_actual * num_ch);
    if (metrics->theoretical_sps_per_channel > 0) {
         printf("vs Theoretical (based on DRATE): %.1f%% efficiency\n", (final_rate_actual / metrics->theoretical_sps_per_channel) * 100);
    }
    free(ADC);
}

void benchmark_comparison(UBYTE *channels, int num_ch, ADS1256_DRATE current_drate, ADS1256_GAIN current_gain) {
    printf("\n=== Benchmarking: Optimized vs Fast Mode (%d Channels) ===\n", num_ch);

    const int benchmark_duration_sec = 10;
    printf("Running %d-second benchmark for each mode...\n\n", benchmark_duration_sec);

    UDOUBLE *ADC_bm = malloc(num_ch * sizeof(UDOUBLE));
    if (!ADC_bm) {
        perror("Failed to allocate memory for ADC readings in benchmark");
        return;
    }

    // Test optimized mode
    printf("1. Testing OPTIMIZED mode (full settling)...\n");
    ADS1256_InitPerformanceMonitoring(current_drate); // Ensure fresh start for metrics
    performance_metrics_t *opt_metrics = ADS1256_GetPerformanceMetrics();
    time_t bm_start_time = time(NULL);
    while((time(NULL) - bm_start_time) < benchmark_duration_sec && running) {
        ADS1256_GetNChannels_Optimized(ADC_bm, channels, num_ch, 5); // 5 settling cycles
    }
    // After loop, metrics are updated. Forcing one last update for print is fine.
    ADS1256_PrintPerformanceReport(); // Print the report for optimized mode
    double optimized_actual_sps_ch = opt_metrics->actual_avg_sps_per_channel;
    double optimized_efficiency = opt_metrics->efficiency_percent;
    printf("\n"); // Add newline after report

    printf("   Optimized: %.1f SPS/ch, %.1f%% efficiency\n",
           optimized_actual_sps_ch, optimized_efficiency);
    
    // Test fast mode
    printf("\n2. Testing FAST mode (reduced settling)...\n");
    // For fast mode, we calculate manually as it doesn't use the same performance struct updates.
    // Or, we could adapt InitPerformanceMonitoring for it if it makes sense.
    // For now, manual calculation as in test_n_channel_fast.
    struct timeval fast_bm_start_tv, fast_bm_end_tv;
    unsigned long fast_total_scans_bm = 0;
    gettimeofday(&fast_bm_start_tv, NULL);
    bm_start_time = time(NULL); // Reset timer for fast mode duration
    while((time(NULL) - bm_start_time) < benchmark_duration_sec && running) {
        ADS1256_GetNChannels_Fast(ADC_bm, channels, num_ch);
        fast_total_scans_bm++;
    }
    gettimeofday(&fast_bm_end_tv, NULL);
    double fast_elapsed_sec = (fast_bm_end_tv.tv_sec - fast_bm_start_tv.tv_sec) + 
                              (fast_bm_end_tv.tv_usec - fast_bm_start_tv.tv_usec) / 1000000.0;
    double fast_actual_sps_ch = (fast_elapsed_sec > 0) ? (fast_total_scans_bm / fast_elapsed_sec) : 0;
    double fast_efficiency_bm = (opt_metrics->theoretical_sps_per_channel > 0) ? 
                                (fast_actual_sps_ch / opt_metrics->theoretical_sps_per_channel) * 100 : 0;
    printf("   Fast: %.1f SPS/ch, %.1f%% efficiency (vs theoretical DRATE limit)\n\n",
            fast_actual_sps_ch, fast_efficiency_bm);

    printf("\n=== Comparison Results ===\n");
    if (optimized_actual_sps_ch > 0) {
        printf("Speed gain (Fast vs Optimized): %.1f%%\n", 
               ((fast_actual_sps_ch / optimized_actual_sps_ch) - 1) * 100);
    } else {
        printf("Speed gain (Fast vs Optimized): N/A (Optimized rate was zero or too low)\n");
    }
    // Recommendation based on this benchmark
    printf("Recommendation: ");
    if (fast_actual_sps_ch > optimized_actual_sps_ch && fast_efficiency_bm > optimized_efficiency * 0.8) {
         printf("Consider FAST mode for higher throughput if minor accuracy trade-off is acceptable.\n");
    } else if (optimized_efficiency > 80) {
        printf("OPTIMIZED mode offers good accuracy and efficiency.\n");
    } else {
        printf("Review setup; both modes may be underperforming. FAST mode is quicker.\n");
    }
    free(ADC_bm);
}

const char* drate_to_string(ADS1256_DRATE drate) {
    switch (drate) {
        case ADS1256_30000SPS: return "30000 SPS";
        case ADS1256_15000SPS: return "15000 SPS";
        case ADS1256_7500SPS:  return "7500 SPS";
        case ADS1256_3750SPS:  return "3750 SPS";
        case ADS1256_2000SPS:  return "2000 SPS";
        case ADS1256_1000SPS:  return "1000 SPS";
        case ADS1256_500SPS:   return "500 SPS";
        case ADS1256_100SPS:   return "100 SPS";
        case ADS1256_60SPS:    return "60 SPS";
        case ADS1256_50SPS:    return "50 SPS";
        case ADS1256_30SPS:    return "30 SPS";
        case ADS1256_25SPS:    return "25 SPS";
        case ADS1256_15SPS:    return "15 SPS";
        case ADS1256_10SPS:    return "10 SPS";
        case ADS1256_5SPS:     return "5 SPS";
        case ADS1256_2d5SPS:   return "2.5 SPS";
        default: return "Unknown DRATE";
    }
}

const char* gain_to_string(ADS1256_GAIN gain) {
    switch (gain) {
        case ADS1256_GAIN_1  : return "GAIN 1";
        case ADS1256_GAIN_2  : return "GAIN 2";
        case ADS1256_GAIN_4  : return "GAIN 4";
        case ADS1256_GAIN_8  : return "GAIN 8";
        case ADS1256_GAIN_16 : return "GAIN 16";
        case ADS1256_GAIN_32 : return "GAIN 32";
        case ADS1256_GAIN_64 : return "GAIN 64";
        default: return "Unknown GAIN";
    }
}

void select_drate_setting(ADS1256_DRATE *current_drate_ptr) {
    int choice;
    printf("\n\n--- Select DRATE ---\n");
    for (int i = 0; i < ADS1256_DRATE_MAX; i++) {
        printf("%d. %s\n", i, drate_to_string((ADS1256_DRATE)i));
    }
    printf("Current DRATE: %s\n", drate_to_string(*current_drate_ptr));
    printf("Enter choice (0-%d): ", ADS1256_DRATE_MAX - 1);
    if (scanf("%d", &choice) == 1 && choice >= 0 && choice < ADS1256_DRATE_MAX) {
        if (*current_drate_ptr != (ADS1256_DRATE)choice) {
            *current_drate_ptr = (ADS1256_DRATE)choice;
            adc_reinit_required = 1;
            printf("DRATE set to %s. ADC will be re-initialized before next test.\n", drate_to_string(*current_drate_ptr));
        } else {
            printf("DRATE unchanged.\n");
        }
    } else {
        printf("Invalid DRATE choice.\n");
        while(getchar() != '\n'); // Clear invalid input
    }
    printf("\n");
}

void select_gain_setting(ADS1256_GAIN *current_gain_ptr) {
    int choice;
    printf("\n\n--- Select GAIN ---\n");
    for (int i = 0; i <= ADS1256_GAIN_64; i++) { // Assuming GAIN_64 is the max enum value
        printf("%d. %s\n", i, gain_to_string((ADS1256_GAIN)i));
    }
    printf("Current GAIN: %s\n", gain_to_string(*current_gain_ptr));
    printf("Enter choice (0-%d): ", ADS1256_GAIN_64);
     if (scanf("%d", &choice) == 1 && choice >= 0 && choice <= ADS1256_GAIN_64) {
        if (*current_gain_ptr != (ADS1256_GAIN)choice) {
            *current_gain_ptr = (ADS1256_GAIN)choice;
            adc_reinit_required = 1;
            printf("GAIN set to %s. ADC will be re-initialized before next test.\n", gain_to_string(*current_gain_ptr));
        } else {
            printf("GAIN unchanged.\n");
        }
    } else {
        printf("Invalid GAIN choice.\n");
        while(getchar() != '\n'); // Clear invalid input
    }
    printf("\n");
}

void configure_test_channels(UBYTE *ch_array, int *num_ch_ptr) {
    int new_num_channels;
    printf("\n\n--- Configure Test Channels ---\n");
    printf("Current channels (%d): ", *num_ch_ptr);
    for (int i = 0; i < *num_ch_ptr; i++) {
        printf("AIN%d ", ch_array[i]);
    }
    printf("\n\n");

    printf("Enter number of channels to test (1-%d): ", NUM_SINGLE_ENDED_CHANNELS);
    if (scanf("%d", &new_num_channels) == 1 && new_num_channels > 0 && new_num_channels <= NUM_SINGLE_ENDED_CHANNELS) {
        printf("Enter %d channel numbers (0-7), separated by spaces: ", new_num_channels);
        UBYTE temp_channels[NUM_SINGLE_ENDED_CHANNELS]; // Temporary array for new channels
        for (int i = 0; i < new_num_channels; i++) {
            int ch_val;
            if (scanf("%d", &ch_val) == 1 && ch_val >= 0 && ch_val < NUM_SINGLE_ENDED_CHANNELS) {
                // Basic check for duplicate channels (can be improved)
                int duplicate = 0;
                for(int j=0; j<i; ++j) {
                    if(temp_channels[j] == (UBYTE)ch_val) { // Check against temp_channels
                        duplicate = 1;
                        break;
                    }
                }
                if(duplicate) {
                    printf("Channel %d is a duplicate, please enter a different channel.\n", ch_val);
                    i--; // Retry current channel input
                } else {
                    temp_channels[i] = (UBYTE)ch_val; // Store in temp array
                }
            } else {
                printf("Invalid channel number. Configuration aborted.\n");
                while(getchar() != '\n'); // Clear invalid input
                return;
            }
        }
        // If all inputs are valid, copy to the main array
        for(int i=0; i<new_num_channels; ++i) {
            ch_array[i] = temp_channels[i];
        }
        *num_ch_ptr = new_num_channels;
        printf("Channels configured. New configuration: ");
        for (int i = 0; i < *num_ch_ptr; i++) {
            printf("AIN%d ", ch_array[i]);
        }
        printf("\n");
        // Re-print channel config with new settings
        print_channel_config(ch_array, *num_ch_ptr);
    } else {
        printf("Invalid number of channels.\n");
        while(getchar() != '\n'); // Clear invalid input
    }
    printf("\n");
}


int main(void) {
    print_usage_info();
    
    if(DEV_ModuleInit() != 0) {
        printf("❌ Failed to initialize device\n");
        return 1;
    }

    signal(SIGINT, Handler);

    // Initial ADC initialization with default settings
    ADS1256_SCAN_MODE scan_mode_setting = SCAN_MODE_SINGLE_ENDED;
    if(ADS1256_init(drate_setting, gain_setting, scan_mode_setting) == 1) {
        printf("❌ ADS1256 initial initialization failed\n");
        DEV_ModuleExit();
        return 1;
    }
    printf("✅ ADS1256 initialized successfully (Rate: %s, Gain: %s, Mode: Single-Ended)\n", 
            drate_to_string(drate_setting), gain_to_string(gain_setting));
    adc_reinit_required = 0; // Reset flag after initial init

    print_channel_config(selected_channels, num_selected_channels);

    int choice;
    do {
        printf("\n\nSelect test mode or configuration option:\n");
        printf("1. Optimized %d-channel test (Current: %s, %s)\n", num_selected_channels, drate_to_string(drate_setting), gain_to_string(gain_setting));
        printf("2. Fast %d-channel test (Current: %s, %s)\n", num_selected_channels, drate_to_string(drate_setting), gain_to_string(gain_setting));
        printf("3. Benchmark comparison (%d-channel) (Current: %s, %s)\n", num_selected_channels, drate_to_string(drate_setting), gain_to_string(gain_setting));
        printf("4. Change display mode (current: %s)\n", display_mode == 1 ? "Continuous" : "Summary only");
        printf("5. Configure Test Channels (Currently %d channels: ", num_selected_channels);
        for(int i=0; i<num_selected_channels; ++i) printf("AIN%d ", selected_channels[i]);
        printf(")\n");
        printf("6. Change DRATE (Current: %s)\n", drate_to_string(drate_setting));
        printf("7. Change GAIN (Current: %s)\n", gain_to_string(gain_setting));
        printf("8. Exit\n");
        printf("Choice (1-8): ");
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n'); // Clear invalid input
            choice = 0; // Invalid choice
        }

        running = 1; // Reset running flag for tests that use it

        // Re-initialize ADC if settings changed
        if (adc_reinit_required && (choice == 1 || choice == 2 || choice == 3)) {
            printf("\nRe-initializing ADS1256 with new settings (DRATE: %s, GAIN: %s)...\n",
                   drate_to_string(drate_setting), gain_to_string(gain_setting));
            if (ADS1256_init(drate_setting, gain_setting, scan_mode_setting) == 1) {
                printf("❌ ADS1256 re-initialization failed. Aborting test.\n");
                DEV_ModuleExit(); // Or handle error more gracefully
                return 1; 
            }
            printf("✅ ADS1256 re-initialized successfully.\n");
            adc_reinit_required = 0; // Reset flag
            print_channel_config(selected_channels, num_selected_channels); // Show updated config
        }


        switch(choice) {
            case 1:
                test_n_channel_optimized(selected_channels, num_selected_channels, drate_setting, gain_setting);
                break;
            case 2:
                test_n_channel_fast(selected_channels, num_selected_channels, drate_setting, gain_setting);
                break;
            case 3:
                benchmark_comparison(selected_channels, num_selected_channels, drate_setting, gain_setting);
                break;
            case 4:
                display_mode = (display_mode == 1) ? 2 : 1;
                printf("Display mode set to: %s\n",
                       display_mode == 1 ? "Continuous data" : "Summary reports only");
                break;
            case 5:
                configure_test_channels(selected_channels, &num_selected_channels);
                break;
            case 6:
                select_drate_setting(&drate_setting);
                break;
            case 7:
                select_gain_setting(&gain_setting);
                break;
            case 8:
                printf("Exiting...\n\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n\n");
                break;
        }
    } while (choice != 8 && running); // Main loop exits on choice 8 or if running is set to 0 by signal handler

    DEV_ModuleExit();
    printf("Program terminated.\n\n");
    return 0;
}