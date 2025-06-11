#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "ADS1256.h"
#include "stdio.h"

// Global control variables
static volatile int running = 1;
static int display_mode = 1; // 1=continuous, 2=summary only

void Handler(int signo) {
    running = 0;
    printf("\r\n=== Shutting down gracefully ===\r\n");
}

void print_usage_info(void) {
    printf("\n=== ADS1256 4-Channel Optimized Performance Test ===\n");
    printf("This program demonstrates optimized 4-channel ADC sampling with:\n");
    printf("- Proper settling time handling (5 DRDY periods)\n");
    printf("- Real-time performance metrics vs theoretical limits\n");
    printf("- Efficiency monitoring and optimization feedback\n");
    printf("- Fixed sign extension for accurate negative voltages\n\n");
}

void print_channel_config(UBYTE *channels) {
    printf("Channel Configuration:\n");
    for(int i = 0; i < 4; i++) {
        printf("  Slot %d -> AIN%d\n", i, channels[i]);
    }
    printf("All channels: Single-ended, referenced to AINCOM\n");
    printf("Voltage range: ±2.5V (5V reference, gain=1)\n\n");
}

void test_4channel_optimized(UBYTE *channels) {
    printf("=== Testing 4-Channel Optimized (Full Settling Time) ===\n");
    
    UDOUBLE ADC[4];
    performance_metrics_t *metrics;
    int sample_count = 0;
    time_t last_report = time(NULL);
    
    // Initialize performance monitoring
    ADS1256_InitPerformanceMonitoring_4Ch();
    
    printf("Starting optimized 4-channel sampling...\n");
    printf("Expected: ~8,748 SPS per channel (~34,992 total)\n");
    printf("Press Ctrl+C to stop and view final report\n\n");
    
    if(display_mode == 1) {
        printf("Ch%d(V)\t\tCh%d(V)\t\tCh%d(V)\t\tCh%d(V)\t\tRate/Ch\t\tEff%%\t\tStatus\n", 
               channels[0], channels[1], channels[2], channels[3]);
        printf("------------------------------------------------------------------------\n");
    }
    
    while(running) {
        // Perform 4-channel acquisition
        ADS1256_Get4Channels_Optimized(ADC, channels);
        sample_count++;
        metrics = ADS1256_GetPerformanceMetrics();
        
        if(display_mode == 1) {
            // Display current values every 10th sample to avoid overwhelming output
            if(sample_count % 10 == 0) {
                for(int i = 0; i < 4; i++) {
                    float voltage = ADS1256_RawToVoltage(ADC[i]);
                    printf("%.4f\t\t", voltage);
                }
                
                printf("%.1f\t\t%.1f\t\t", 
                       metrics->actual_per_channel, metrics->efficiency_percent);
                
                // Status indicator
                if(metrics->efficiency_percent > 85) {
                    printf("EXCELLENT\r");
                } else if(metrics->efficiency_percent > 70) {
                    printf("GOOD\r");
                } else if(metrics->efficiency_percent > 50) {
                    printf("FAIR\r");
                } else {
                    printf("POOR\r");
                }
                fflush(stdout);
            }
        }
        
        // Print summary report every 5 seconds
        time_t current_time = time(NULL);
        if(current_time - last_report >= 5) {
            if(display_mode == 2) {
                printf("\n--- 5-Second Update ---\n");
                printf("Scans completed: %lu\n", metrics->total_scans);
                printf("Rate per channel: %.1f SPS (Target: %.0f)\n", 
                       metrics->actual_per_channel, metrics->theoretical_max_per_channel);
                printf("Total rate: %.1f SPS (Target: %.0f)\n", 
                       metrics->actual_total, metrics->theoretical_total);
                printf("Efficiency: %.1f%%\n", metrics->efficiency_percent);
                
                // Performance guidance
                if(metrics->efficiency_percent < 70) {
                    printf("⚠️  Performance below expected levels. Consider:\n");
                    printf("   - Checking SPI bus speed and stability\n");
                    printf("   - Reducing system load\n");
                    printf("   - Verifying DRDY signal integrity\n");
                } else if(metrics->efficiency_percent > 90) {
                    printf("✅ Excellent performance - near theoretical limits!\n");
                }
                printf("\n");
            }
            last_report = current_time;
        }
        
        // Small delay to prevent CPU overload
        usleep(100);
    }
    
    printf("\n\n");
    ADS1256_PrintPerformanceReport();
}

void test_4channel_fast(UBYTE *channels) {
    printf("=== Testing 4-Channel Fast (Reduced Settling Time) ===\n");
    
    UDOUBLE ADC[4];
    struct timeval start_time, current_time;
    unsigned long scan_count = 0;
    
    gettimeofday(&start_time, NULL);
    
    printf("Starting fast 4-channel sampling...\n");
    printf("Expected: Higher rate than optimized, but potentially less accurate\n");
    printf("Press Ctrl+C to stop\n\n");
    
    printf("Ch%d(V)\t\tCh%d(V)\t\tCh%d(V)\t\tCh%d(V)\t\tRate/Ch\n", 
           channels[0], channels[1], channels[2], channels[3]);
    printf("--------------------------------------------------------\n");
    
    while(running) {
        ADS1256_Get4Channels_Fast(ADC, channels);
        scan_count++;
        
        // Display every 20th sample
        if(scan_count % 20 == 0) {
            gettimeofday(&current_time, NULL);
            double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                           (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
            double rate_per_channel = scan_count / elapsed;
            
            for(int i = 0; i < 4; i++) {
                float voltage = ADS1256_RawToVoltage(ADC[i]);
                printf("%.4f\t\t", voltage);
            }
            printf("%.1f\r", rate_per_channel);
            fflush(stdout);
        }
        
        usleep(50);
    }
    
    gettimeofday(&current_time, NULL);
    double total_elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                          (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
    double final_rate = scan_count / total_elapsed;
    
    printf("\n\n=== Fast Mode Results ===\n");
    printf("Total scans: %lu\n", scan_count);
    printf("Runtime: %.2f seconds\n", total_elapsed);
    printf("Rate per channel: %.1f SPS\n", final_rate);
    printf("Total rate: %.1f SPS\n", final_rate * 4);
    printf("vs Theoretical: %.1f%% efficiency\n", (final_rate / 8748.0) * 100);
}

void benchmark_comparison(UBYTE *channels) {
    printf("=== Benchmarking: Optimized vs Fast Mode ===\n");
    
    const int test_duration = 10; // seconds
    printf("Running %d-second benchmark for each mode...\n\n", test_duration);
    
    // Test optimized mode
    printf("1. Testing OPTIMIZED mode (full settling)...\n");
    ADS1256_InitPerformanceMonitoring_4Ch();
    
    time_t start_time = time(NULL);
    UDOUBLE ADC[4];
    
    while((time(NULL) - start_time) < test_duration) {
        ADS1256_Get4Channels_Optimized(ADC, channels);
    }
    
    performance_metrics_t *opt_metrics = ADS1256_GetPerformanceMetrics();
    printf("   Optimized: %.1f SPS/ch, %.1f%% efficiency\n", 
           opt_metrics->actual_per_channel, opt_metrics->efficiency_percent);
    
    // Test fast mode
    printf("2. Testing FAST mode (reduced settling)...\n");
    start_time = time(NULL);
    unsigned long fast_scans = 0;
    
    while((time(NULL) - start_time) < test_duration) {
        ADS1256_Get4Channels_Fast(ADC, channels);
        fast_scans++;
    }
    
    double fast_rate = fast_scans / (double)test_duration;
    double fast_efficiency = (fast_rate / 8748.0) * 100;
    printf("   Fast: %.1f SPS/ch, %.1f%% efficiency\n", fast_rate, fast_efficiency);
    
    // Comparison
    printf("\n=== Comparison Results ===\n");
    printf("Speed gain (Fast vs Optimized): %.1f%%\n", 
           ((fast_rate / opt_metrics->actual_per_channel) - 1) * 100);
    printf("Recommendation: ");
    if(fast_efficiency > 100) {
        printf("Use FAST mode - exceeds theoretical limits!\n");
    } else if(opt_metrics->efficiency_percent > 85) {
        printf("Use OPTIMIZED mode - excellent accuracy\n");
    } else {
        printf("Use FAST mode - better performance\n");
    }
}

int main(void) {
    print_usage_info();
    
    if(DEV_ModuleInit() != 0) {
        printf("❌ Failed to initialize device\n");
        return 1;
    }

    signal(SIGINT, Handler);

    if(ADS1256_init() == 1) {
        printf("❌ ADS1256 initialization failed\n");
        DEV_ModuleExit();
        return 1;
    }
    
    printf("✅ ADS1256 initialized successfully\n");

    // Configure 4 channels - you can modify these
    UBYTE selected_channels[4] = {0, 2, 4, 6}; // Every other channel for minimal crosstalk
    print_channel_config(selected_channels);

    int choice;
    printf("Select test mode:\n");
    printf("1. Optimized 4-channel (full settling, max accuracy)\n");
    printf("2. Fast 4-channel (reduced settling, higher speed)\n");
    printf("3. Benchmark comparison\n");
    printf("4. Change display mode\n");
    printf("5. Exit\n");
    printf("Choice (1-5): ");
    
    while(scanf("%d", &choice) == 1) {
        switch(choice) {
            case 1:
                test_4channel_optimized(selected_channels);
                break;
            case 2:
                test_4channel_fast(selected_channels);
                break;
            case 3:
                benchmark_comparison(selected_channels);
                break;
            case 4:
                display_mode = (display_mode == 1) ? 2 : 1;
                printf("Display mode set to: %s\n", 
                       display_mode == 1 ? "Continuous" : "Summary only");
                break;
            case 5:
                printf("Exiting...\n");
                DEV_ModuleExit();
                return 0;
            default:
                printf("Invalid choice\n");
                break;
        }
        
        if(choice >= 1 && choice <= 3) {
            running = 1; // Reset for next test
            printf("\nSelect test mode:\n");
            printf("1. Optimized 4-channel\n");
            printf("2. Fast 4-channel\n");
            printf("3. Benchmark comparison\n");
            printf("4. Change display mode (current: %s)\n", 
                   display_mode == 1 ? "Continuous" : "Summary");
            printf("5. Exit\n");
            printf("Choice (1-5): ");
        }
    }

    DEV_ModuleExit();
    return 0;
}