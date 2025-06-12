/** 
 * @file ADS1256.c
 * @brief Driver for the ADS1256 24-bit Analog-to-Digital Converter.
 *
 * This file provides functions to initialize, configure, and read data from
 * the ADS1256 ADC. It supports single-ended and differential input modes,
 * various gain settings, and data rates. It also includes functions for
 * performance monitoring and conversion of raw ADC values to voltage.
 */

#include "ADS1256.h"
#include <stdio.h> 
#include <sys/time.h> 

// Global variable to store the current scan mode
UBYTE ScanMode = SCAN_MODE_SINGLE_ENDED; // Default to single-ended

// Performance tracking structure instance
static performance_metrics_t perf_metrics = {0};

/**
 * @brief Resets the ADS1256 module.
 */
static void ADS1256_reset(void)
{
    DEV_Digital_Write(DEV_RST_PIN, HIGH);
    DEV_Delay_ms(200);
    DEV_Digital_Write(DEV_RST_PIN, LOW); 
    DEV_Delay_ms(200);
    DEV_Digital_Write(DEV_RST_PIN, HIGH);
    DEV_Delay_ms(200); 
}

/**
 * @brief Sends a command to the ADS1256.
 * @param Cmd The command byte to send (from ADS1256_CMD enum).
 */
static void ADS1256_WriteCmd(UBYTE Cmd)
{
    DEV_Digital_Write(DEV_CS_PIN, LOW);
    DEV_SPI_WriteByte(Cmd);      
    DEV_Digital_Write(DEV_CS_PIN, HIGH);
}

/**
 * @brief Writes data to a specified register in the ADS1256.
 * @param Reg The target register address (from ADS1256_REG enum).
 * @param Data The byte of data to write to the register.
 */
static void ADS1256_WriteReg(UBYTE Reg, UBYTE Data)
{
    DEV_Digital_Write(DEV_CS_PIN, LOW);  
    DEV_SPI_WriteByte(CMD_WREG | Reg); 
    DEV_SPI_WriteByte(0x00);           
    DEV_SPI_WriteByte(Data);           
    DEV_Digital_Write(DEV_CS_PIN, HIGH);  
}

/**
 * @brief Reads data from a specified register in the ADS1256.
 * @param Reg The target register address (from ADS1256_REG enum).
 * @return The byte of data read from the register.
 */
static UBYTE ADS1256_Read_data(UBYTE Reg)
{
    UBYTE temp = 0;
    DEV_Digital_Write(DEV_CS_PIN, LOW);  
    DEV_SPI_WriteByte(CMD_RREG | Reg); 
    DEV_SPI_WriteByte(0x00);           
    // DEV_Delay_ms(1); // Small delay for command processing. Datasheet t6 is ~0.5us. SPI driver might handle this.
    temp = DEV_SPI_ReadByte();         
    DEV_Digital_Write(DEV_CS_PIN, HIGH);  
    return temp;
}

/**
 * @brief Waits for the Data Ready (DRDY) pin to go low.
 */
static void ADS1256_WaitDRDY(void)
{
    UDOUBLE i = 0;
    const UDOUBLE timeout_cycles = 4000000; 
    for(i = 0; i < timeout_cycles; i++) {
        if(DEV_Digital_Read(DEV_DRDY_PIN) == LOW) {
            return; 
        }
    }
    Debug("ADS1256_WaitDRDY: Timeout!\n");
}

/**
 * @brief Reads the Chip ID from the ADS1256.
 * @return The 4-bit chip ID. Expected value is 3 for ADS1256.
 */
UBYTE ADS1256_ReadChipID(void)
{
    UBYTE id;
    id = ADS1256_Read_data(REG_STATUS);
    return (id >> 4); 
}

/**
 * @brief Configures the ADS1256 ADC settings.
 * @param gain The PGA gain setting (ADS1256_GAIN enum).
 * @param drate The data rate (ADS1256_DRATE enum).
 */
void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate)
{
    ADS1256_WaitDRDY();

    UBYTE status_reg = (0 << 7) | // ORDER: MSB first
                       (0 << 6) | // ACAL: Auto-Calibration disabled
                       (1 << 5) | // BUFEN: Analog input buffer enabled
                       (0 << 4) | // DRDY_MODE: DRDY pin reflects DRDY state
                       (0 << 3) | // CLKOUT: Clock output disabled
                       (0 << 2) | // SDCS: Sensor detect current sources off
                       (0 << 0);  // Lower 2 bits are read-only

    UBYTE mux_reg = 0x01; // Default: AIN0 positive, AINCOM negative

    UBYTE adcon_reg = (0 << 5) | // CLK_OFF: Internal oscillator ON
                      (0 << 3) | // SDCS_MODE: Sensor detect current sources controlled by SDCS bits
                      (gain & 0x07); // PGA: Gain setting

    UBYTE drate_reg = ADS1256_DRATE_E[drate];

    DEV_Digital_Write(DEV_CS_PIN, LOW);
    DEV_SPI_WriteByte(CMD_WREG | REG_STATUS); 
    DEV_SPI_WriteByte(0x03); // Write 4 registers (STATUS, MUX, ADCON, DRATE)

    DEV_SPI_WriteByte(status_reg);
    DEV_SPI_WriteByte(mux_reg);    
    DEV_SPI_WriteByte(adcon_reg);
    DEV_SPI_WriteByte(drate_reg);

    DEV_Digital_Write(DEV_CS_PIN, HIGH);
    DEV_Delay_ms(1); 
}

/**
 * @brief Sets the active input channel for single-ended measurements.
 * @param Channel The channel number (0-7 for AIN0-AIN7 vs AINCOM).
 */
static void ADS1256_SetChannel(UBYTE Channel)
{
    if (Channel > 7) {
        Debug("ADS1256_SetChannel: Invalid channel %d\n", Channel);
        return;
    }
    ADS1256_WriteReg(REG_MUX, (Channel << 4) | 0x08); // PSEL=Channel, NSEL=AINCOM
}

/**
 * @brief Sets the active input channel pair for differential measurements.
 * @param DiffPairIndex The differential channel pair index (0-3).
 */
static void ADS1256_SetDiffChannel(UBYTE DiffPairIndex) 
{
    UBYTE mux_val = 0;
    switch (DiffPairIndex) {
        case 0: mux_val = (ADS1256_MUX_AIN0 << 4) | ADS1256_MUX_AIN1; break; // AIN0 - AIN1
        case 1: mux_val = (ADS1256_MUX_AIN2 << 4) | ADS1256_MUX_AIN3; break; // AIN2 - AIN3
        case 2: mux_val = (ADS1256_MUX_AIN4 << 4) | ADS1256_MUX_AIN5; break; // AIN4 - AIN5
        case 3: mux_val = (ADS1256_MUX_AIN6 << 4) | ADS1256_MUX_AIN7; break; // AIN6 - AIN7
        // Additional combinations if needed, e.g., AIN1-AIN0
        // case 4: mux_val = (ADS1256_MUX_AIN1 << 4) | ADS1256_MUX_AIN0; break; // AIN1 - AIN0 
        default:
            Debug("ADS1256_SetDiffChannel: Invalid differential pair index %d\n", DiffPairIndex);
            return;
    }
    ADS1256_WriteReg(REG_MUX, mux_val);
}

/**
 * @brief Initializes the ADS1256 module.
 * @param drate The desired data rate (ADS1256_DRATE enum).
 * @param gain The desired PGA gain (ADS1256_GAIN enum).
 * @param scan_mode The desired scan mode (SCAN_MODE_SINGLE_ENDED or SCAN_MODE_DIFFERENTIAL_INPUTS).
 * @return 0 on successful initialization, 1 on failure.
 */
UBYTE ADS1256_init(ADS1256_DRATE drate, ADS1256_GAIN gain, ADS1256_SCAN_MODE scan_mode)
{
    ScanMode = scan_mode;
    ADS1256_reset();
    UBYTE chip_id = ADS1256_ReadChipID();
    if (chip_id == ADS1256_ID) { 
        Debug("ADS1256_init: Chip ID read success (ID: %d)\r\n", chip_id);
    } else {
        fprintf(stderr, "ADS1256_init: Chip ID read failed (Expected: %d, Got: %d)\r\n", ADS1256_ID, chip_id);
        return 1; 
    }
    ADS1256_ConfigADC(gain, drate); 
    // ADS1256_WriteCmd(CMD_SELFCAL); // Optional: Perform self-calibration
    // DEV_Delay_ms(200); 
    // ADS1256_WaitDRDY(); 

    Debug("ADS1256_init: Initialization complete. Mode: %s\n", 
          (ScanMode == SCAN_MODE_SINGLE_ENDED) ? "Single-Ended" : "Differential");
    return 0; 
}

/**
 * @brief Reads the raw 24-bit ADC conversion data.
 * @return The raw 24-bit ADC data, sign-extended to UDOUBLE (uint32_t).
 */
static UDOUBLE ADS1256_read_ADC_Data(void)
{
    UDOUBLE read_value = 0;
    UBYTE buf[3] = {0, 0, 0};

    DEV_Digital_Write(DEV_CS_PIN, LOW);
    DEV_SPI_WriteByte(CMD_RDATA);    
    // DEV_Delay_ms(1); // Small delay, t_6 is ~3.125us for 7.68MHz clock.

    buf[0] = DEV_SPI_ReadByte(); 
    buf[1] = DEV_SPI_ReadByte(); 
    buf[2] = DEV_SPI_ReadByte(); 
    DEV_Digital_Write(DEV_CS_PIN, HIGH);

    read_value = ((UDOUBLE)buf[0] << 16) | ((UDOUBLE)buf[1] << 8) | (UDOUBLE)buf[2];

    if (read_value & 0x00800000) {
        read_value |= 0xFF000000; 
    }
    return read_value;
}

/**
 * @brief Gets the ADC conversion value for a specified channel.
 * @param Channel The channel number (0-7 for single-ended, 0-3 for differential pair index).
 * @return The raw ADC value for the channel, sign-extended.
 */
UDOUBLE ADS1256_GetChannelValue(UBYTE Channel) 
{
    UDOUBLE value = 0;

    if (ScanMode == SCAN_MODE_SINGLE_ENDED) { 
        if (Channel >= NUM_SINGLE_ENDED_CHANNELS) {
            Debug("ADS1256_GetChannelValue: Invalid single-ended channel %d\n", Channel);
            return 0;
        }
        ADS1256_SetChannel(Channel);
    } else { // SCAN_MODE_DIFFERENTIAL_INPUTS
        if (Channel >= NUM_DIFFERENTIAL_PAIRS) {
            Debug("ADS1256_GetChannelValue: Invalid differential pair index %d\n", Channel);
            return 0;
        }
        ADS1256_SetDiffChannel(Channel);
    }

    ADS1256_WriteCmd(CMD_SYNC);  
    ADS1256_WriteCmd(CMD_WAKEUP);

    ADS1256_WaitDRDY(); 
    value = ADS1256_read_ADC_Data();

    return value;
}

/**
 * @brief Reads ADC values from all relevant channels based on ScanMode.
 * @param ADC_Value Pointer to an array where the ADC values will be stored.
 *                  Size should be NUM_SINGLE_ENDED_CHANNELS or NUM_DIFFERENTIAL_PAIRS.
 */
void ADS1256_GetAllChannels(UDOUBLE *ADC_Value)
{
    UBYTE i;
    UBYTE num_channels_to_read = (ScanMode == SCAN_MODE_SINGLE_ENDED) ? 
                                 NUM_SINGLE_ENDED_CHANNELS : NUM_DIFFERENTIAL_PAIRS;

    for (i = 0; i < num_channels_to_read; i++) {
        ADC_Value[i] = ADS1256_GetChannelValue(i);
    }
}

// --- Performance Monitoring and Optimized Read Functions ---

/**
 * @brief Fixes sign extension for a raw 24-bit ADC value.
 * @param raw_value The raw 24-bit ADC value.
 * @return The sign-extended 32-bit value.
 */
static UDOUBLE ADS1256_fix_sign_extension(UDOUBLE raw_value) {
    if (raw_value & 0x00800000) { 
        raw_value |= 0xFF000000;  
    }
    return raw_value;
}

/**
 * @brief Reads ADC data after ensuring proper settling time.
 * @param num_settling_drdy_cycles Number of DRDY cycles to wait for settling.
 * @return Settled raw ADC data, sign-extended.
 */
static UDOUBLE ADS1256_read_ADC_Data_settled(UBYTE num_settling_drdy_cycles)
{
    UBYTE buf[3] = {0, 0, 0};
    UDOUBLE read_value = 0;

    if (num_settling_drdy_cycles == 0) num_settling_drdy_cycles = 1; 

    for (UBYTE settle_count = 0; settle_count < num_settling_drdy_cycles; settle_count++) {
        ADS1256_WaitDRDY(); 

        if (settle_count == num_settling_drdy_cycles - 1) {
            DEV_Digital_Write(DEV_CS_PIN, LOW);
            DEV_SPI_WriteByte(CMD_RDATA);
            buf[0] = DEV_SPI_ReadByte();
            buf[1] = DEV_SPI_ReadByte();
            buf[2] = DEV_SPI_ReadByte();
            DEV_Digital_Write(DEV_CS_PIN, HIGH);
        }
    }

    read_value = ((UDOUBLE)buf[0] << 16) | ((UDOUBLE)buf[1] << 8) | (UDOUBLE)buf[2];
    return ADS1256_fix_sign_extension(read_value);
}

/**
 * @brief Optimized function to acquire data from a specified list of up to N single-ended channels.
 * @param ADC_Value Pointer to an array to store the read ADC values.
 * @param channels Array of UBYTE specifying the channel numbers (0-7) to read.
 * @param num_channels_to_read The number of channels to read from the `channels` array.
 * @param settling_cycles Number of DRDY cycles to wait for settling after each channel switch.
 */
void ADS1256_GetNChannels_Optimized(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels_to_read, UBYTE settling_cycles)
{
    if (!ADC_Value || !channels || num_channels_to_read == 0) return;
    if (num_channels_to_read > NUM_SINGLE_ENDED_CHANNELS) num_channels_to_read = NUM_SINGLE_ENDED_CHANNELS; 
    if (settling_cycles == 0) settling_cycles = 1; 

    // struct timeval scan_start_tv; // For more detailed per-scan timing if needed
    // gettimeofday(&scan_start_tv, NULL);

    for (UBYTE i = 0; i < num_channels_to_read; i++) {
        UBYTE current_channel = channels[i];

        if (current_channel >= NUM_SINGLE_ENDED_CHANNELS) { 
            ADC_Value[i] = 0; 
            Debug("ADS1256_GetNChannels_Optimized: Invalid channel %d requested.\n", current_channel);
            continue;
        }

        ADS1256_SetChannel(current_channel);

        ADS1256_WriteCmd(CMD_SYNC);
        ADS1256_WriteCmd(CMD_WAKEUP);

        ADC_Value[i] = ADS1256_read_ADC_Data_settled(settling_cycles);
    }

    perf_metrics.total_samples_acquired += num_channels_to_read;
    perf_metrics.total_n_channel_scans++;

    struct timeval current_time_tv;
    gettimeofday(&current_time_tv, NULL);
    double elapsed_seconds = (current_time_tv.tv_sec - perf_metrics.start_time.tv_sec) +
                           (current_time_tv.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;

    if (elapsed_seconds > 0 && perf_metrics.theoretical_sps_per_channel > 0) {
        perf_metrics.actual_avg_sps_total = perf_metrics.total_samples_acquired / elapsed_seconds;
        perf_metrics.actual_avg_sps_per_channel = perf_metrics.actual_avg_sps_total / num_channels_to_read; // Avg over the N channels scanned

        // Efficiency is based on how fast one of the N channels is being sampled vs its theoretical max
        perf_metrics.efficiency_percent = (perf_metrics.actual_avg_sps_per_channel / perf_metrics.theoretical_sps_per_channel) * 100.0;
    }
}

/**
 * @brief Ultra-fast acquisition from a list of up to N single-ended channels with minimal overhead.
 * @param ADC_Value Pointer to an array to store the read ADC values.
 * @param channels Array of UBYTE specifying the channel numbers (0-7) to read.
 * @param num_channels_to_read The number of channels to read from the `channels` array.
 */
void ADS1256_GetNChannels_Fast(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels_to_read)
{
    if (!ADC_Value || !channels || num_channels_to_read == 0) return;
    if (num_channels_to_read > NUM_SINGLE_ENDED_CHANNELS) num_channels_to_read = NUM_SINGLE_ENDED_CHANNELS;

    for (UBYTE i = 0; i < num_channels_to_read; i++) {
        UBYTE current_channel = channels[i];

        if (current_channel >= NUM_SINGLE_ENDED_CHANNELS) {
            ADC_Value[i] = 0;
            Debug("ADS1256_GetNChannels_Fast: Invalid channel %d requested.\n", current_channel);
            continue;
        }
        // Optimized MUX setting: combine WREG MUX, num_reg, data into fewer SPI transactions if possible
        // For now, using standard WriteReg for clarity, can be further optimized.
        ADS1256_WriteReg(REG_MUX, (current_channel << 4) | 0x08); // Channel to AINCOM
        // Minimal delay if any, e.g., DEV_Delay_us(1); // Datasheet t1 is MUX settling time

        ADS1256_WriteCmd(CMD_SYNC);
        ADS1256_WriteCmd(CMD_WAKEUP);

        ADS1256_WaitDRDY(); // Minimal settling (1 DRDY cycle)
        ADC_Value[i] = ADS1256_read_ADC_Data();
    }
    perf_metrics.total_samples_acquired += num_channels_to_read;
    perf_metrics.total_n_channel_scans++; 
}


/**
 * @brief Initializes performance monitoring.
 * @param drate_enum_val The configured data rate (ADS1256_DRATE enum value).
 */
void ADS1256_InitPerformanceMonitoring(ADS1256_DRATE drate_enum_val)
{
    float sps_value = 0.0f;
    switch(drate_enum_val) {
        case ADS1256_30000SPS: sps_value = 30000.0f; break;
        case ADS1256_15000SPS: sps_value = 15000.0f; break;
        case ADS1256_7500SPS:  sps_value = 7500.0f;  break;
        case ADS1256_3750SPS:  sps_value = 3750.0f;  break;
        case ADS1256_2000SPS:  sps_value = 2000.0f;  break;
        case ADS1256_1000SPS:  sps_value = 1000.0f;  break;
        case ADS1256_500SPS:   sps_value = 500.0f;   break;
        case ADS1256_100SPS:   sps_value = 100.0f;   break;
        case ADS1256_60SPS:    sps_value = 60.0f;    break;
        case ADS1256_50SPS:    sps_value = 50.0f;    break;
        case ADS1256_30SPS:    sps_value = 30.0f;    break;
        case ADS1256_25SPS:    sps_value = 25.0f;    break;
        case ADS1256_15SPS:    sps_value = 15.0f;    break;
        case ADS1256_10SPS:    sps_value = 10.0f;    break;
        case ADS1256_5SPS:     sps_value = 5.0f;     break;
        case ADS1256_2d5SPS:   sps_value = 2.5f;     break;
        default: sps_value = 2.5f; 
                 Debug("ADS1256_InitPerformanceMonitoring: Unknown DRATE enum, defaulting to 2.5 SPS.\n");
    }

    perf_metrics.theoretical_sps_per_channel = sps_value;
    perf_metrics.actual_avg_sps_per_channel = 0;
    perf_metrics.actual_avg_sps_total = 0;
    perf_metrics.efficiency_percent = 0;
    perf_metrics.total_samples_acquired = 0;
    perf_metrics.total_n_channel_scans = 0;

    gettimeofday(&perf_metrics.start_time, NULL);

    Debug("=== ADS1256 Performance Monitor Initialized ===\n");
    Debug("Theoretical Max SPS (single channel continuous): %.0f\n",
           perf_metrics.theoretical_sps_per_channel);
}

/**
 * @brief Gets a pointer to the current performance metrics structure.
 * @return Pointer to the static `perf_metrics` structure.
 */
performance_metrics_t* ADS1256_GetPerformanceMetrics(void)
{
    // Ensure latest efficiency is calculated before returning
    struct timeval current_time_tv;
    gettimeofday(&current_time_tv, NULL);
    double elapsed_seconds = (current_time_tv.tv_sec - perf_metrics.start_time.tv_sec) +
                           (current_time_tv.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;

    if (elapsed_seconds > 0 && perf_metrics.theoretical_sps_per_channel > 0 && perf_metrics.total_samples_acquired > 0) {
        // Note: actual_avg_sps_total is total samples / total time.
        // actual_avg_sps_per_channel depends on how many channels were in each scan.
        // For simplicity, if GetNChannels was used with N channels, then actual_avg_sps_per_channel was updated there.
        // If GetChannelValue was used, this calculation might need adjustment based on usage pattern.
        // Here, we recalculate overall average SPS if total_samples_acquired is the primary counter.
        perf_metrics.actual_avg_sps_total = perf_metrics.total_samples_acquired / elapsed_seconds;
        
        // If we assume all scans were of a consistent number of channels (e.g. N from last N-channel call, or 1 for single calls)
        // this efficiency calculation remains an approximation of per-channel throughput vs theoretical.
        if (perf_metrics.total_n_channel_scans > 0) {
             double avg_channels_per_scan_approx = (double)perf_metrics.total_samples_acquired / perf_metrics.total_n_channel_scans;
             if (avg_channels_per_scan_approx > 0) {
                double effective_sps_per_active_channel = perf_metrics.actual_avg_sps_total / avg_channels_per_scan_approx;
                perf_metrics.efficiency_percent = (effective_sps_per_active_channel / perf_metrics.theoretical_sps_per_channel) * 100.0;
             }
        } else if (perf_metrics.total_samples_acquired > 0) { // Fallback if only single samples were taken
            perf_metrics.efficiency_percent = (perf_metrics.actual_avg_sps_total / perf_metrics.theoretical_sps_per_channel) * 100.0;
        }

    } else {
        perf_metrics.efficiency_percent = 0; // Not enough data
    }
    return &perf_metrics;
}

/**
 * @brief Prints a detailed performance report to stdout.
 */
void ADS1256_PrintPerformanceReport(void)
{
    // Update metrics before printing
    ADS1256_GetPerformanceMetrics(); 

    struct timeval current_time_tv;
    gettimeofday(&current_time_tv, NULL);
    double elapsed_seconds = (current_time_tv.tv_sec - perf_metrics.start_time.tv_sec) +
                           (current_time_tv.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;

    printf("\n=== ADS1256 Performance Report ===\n");
    printf("Runtime: %.2f seconds\n", elapsed_seconds);
    printf("Total Samples Acquired: %lu\n", perf_metrics.total_samples_acquired);
    printf("Total N-Channel Scan Operations: %lu\n", perf_metrics.total_n_channel_scans);
    printf("Theoretical Max SPS (single channel continuous): %.0f\n",
           perf_metrics.theoretical_sps_per_channel);
    printf("Actual Average Total SPS (all samples / time): %.1f\n",
           perf_metrics.actual_avg_sps_total);
    // The concept of "actual_avg_sps_per_channel" is tricky if scan types vary.
    // The efficiency calculation gives a better sense of per-channel throughput vs theoretical.
    printf("Overall Efficiency (Effective Per-Channel SPS vs Theoretical Single Channel Max): %.1f%%\n", perf_metrics.efficiency_percent);

    if (perf_metrics.total_samples_acquired == 0) {
        printf("Status: No scan data yet.\n");
    } else if (perf_metrics.efficiency_percent > 90) {
        printf("Status: EXCELLENT - Near theoretical limits for the operation type.\n");
    } else if (perf_metrics.efficiency_percent > 75) {
        printf("Status: GOOD - Acceptable performance.\n");
    } else if (perf_metrics.efficiency_percent > 50) {
        printf("Status: FAIR - Consider optimization if higher throughput needed.\n");
    } else {
        printf("Status: POOR - Significant optimization may be needed or expectations reviewed.\n");
    }
    printf("=============================================\n\n");
}

/**
 * @brief Converts a raw ADC value to voltage.
 * @param raw_value The raw 24-bit ADC code, sign-extended.
 * @param vref_positive Positive reference voltage (e.g., 5.0V for +5V VREF, 2.5V for internal VREF).
 * @param vref_negative Negative reference voltage (e.g., 0.0V for GND, -2.5V for internal VREF).
 * @param gain_enum The PGA gain setting used during the ADC conversion.
 * @return The calculated voltage as a float.
 */
float ADS1256_RawToVoltage(UDOUBLE raw_value, float vref_positive, float vref_negative, ADS1256_GAIN gain_enum)
{
    int32_t signed_adc_code = (int32_t)raw_value; // Cast to signed 32-bit
    float pga_gain_val = 1.0f;

    switch(gain_enum) {
        case ADS1256_GAIN_1  : pga_gain_val = 1.0f; break;
        case ADS1256_GAIN_2  : pga_gain_val = 2.0f; break;
        case ADS1256_GAIN_4  : pga_gain_val = 4.0f; break;
        case ADS1256_GAIN_8  : pga_gain_val = 8.0f; break;
        case ADS1256_GAIN_16 : pga_gain_val = 16.0f; break;
        case ADS1256_GAIN_32 : pga_gain_val = 32.0f; break;
        case ADS1256_GAIN_64 : pga_gain_val = 64.0f; break;
        // default: pga_gain_val = 1.0f; // Or handle error
    }

    // Effective VREF for bipolar operation is (vref_positive - vref_negative)
    float vref_span = vref_positive - vref_negative;

    // Voltage = (ADC_code / 2^23) * (VREF_SPAN / PGA_GAIN)
    // Max positive code 0x7FFFFF (2^23 - 1). Full scale range is 2^24 codes for 2s complement.
    // The scaling factor should be 2^23 = 8388608.0f for bipolar conversion.
    // This formula assumes the ADC code represents a value from -FS to +FS-1LSB.
    // V_in = (CODE / 2^23) * (V_REF / PGA)
    // If using differential VREF (VREFP - VREFN), then V_REF in formula is this span.

    return ((float)signed_adc_code / 8388608.0f) * (vref_span / pga_gain_val);
}
