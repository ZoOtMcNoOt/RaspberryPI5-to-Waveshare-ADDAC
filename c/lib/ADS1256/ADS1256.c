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
#include <stdio.h> // For printf
#include <sys/time.h> // For gettimeofday (used in performance monitoring)

// Global variable to store the current scan mode (single-ended or differential)
UBYTE ScanMode = 0; // 0 for single-ended, 1 for differential

// Performance tracking structure instance
static performance_metrics_t perf_metrics = {0};

/**
 * @brief Resets the ADS1256 module.
 *
 * Toggles the RST pin to reset the ADS1256 to its default state.
 */
static void ADS1256_reset(void)
{
    DEV_Digital_Write(DEV_RST_PIN, 1); // Set RST high
    DEV_Delay_ms(200);                 // Wait 200ms
    DEV_Digital_Write(DEV_RST_PIN, 0); // Set RST low (active reset)
    DEV_Delay_ms(200);                 // Wait 200ms
    DEV_Digital_Write(DEV_RST_PIN, 1); // Set RST high (release reset)
    DEV_Delay_ms(200);                 // Wait for stabilization (optional, but good practice)
}

/**
 * @brief Sends a command to the ADS1256.
 * @param Cmd The command byte to send (from ADS1256_CMD enum).
 */
static void ADS1256_WriteCmd(UBYTE Cmd)
{
    DEV_Digital_Write(DEV_CS_PIN, 0); // Assert CS (active low)
    DEV_SPI_WriteByte(Cmd);           // Send command byte
    DEV_Digital_Write(DEV_CS_PIN, 1); // De-assert CS
}

/**
 * @brief Writes data to a specified register in the ADS1256.
 * @param Reg The target register address (from ADS1256_REG enum).
 * @param Data The byte of data to write to the register.
 */
static void ADS1256_WriteReg(UBYTE Reg, UBYTE Data)
{
    DEV_Digital_Write(DEV_CS_PIN, 0);   // Assert CS
    DEV_SPI_WriteByte(CMD_WREG | Reg);  // Send WREG command + register address
    DEV_SPI_WriteByte(0x00);            // Send number of registers to write - 1 (i.e., 0 for 1 register)
    DEV_SPI_WriteByte(Data);            // Send data byte
    DEV_Digital_Write(DEV_CS_PIN, 1);   // De-assert CS
}

/**
 * @brief Reads data from a specified register in the ADS1256.
 * @param Reg The target register address (from ADS1256_REG enum).
 * @return The byte of data read from the register.
 */
static UBYTE ADS1256_Read_data(UBYTE Reg)
{
    UBYTE temp = 0;
    DEV_Digital_Write(DEV_CS_PIN, 0);   // Assert CS
    DEV_SPI_WriteByte(CMD_RREG | Reg);  // Send RREG command + register address
    DEV_SPI_WriteByte(0x00);            // Send number of registers to read - 1 (i.e., 0 for 1 register)
    // DEV_Delay_ms(1);                 // Small delay for command processing. Datasheet specifies t6 (SCLK falling to DRDY rising) is 4*tCLKIN. For 7.68MHz tCLKIN, this is ~0.5us. A 1ms delay is very conservative.
                                        // For RREG, data is clocked out on subsequent SCLK cycles, so this delay might not be strictly needed here if SPI driver handles it.
    temp = DEV_SPI_ReadByte();          // Read data byte
    DEV_Digital_Write(DEV_CS_PIN, 1);   // De-assert CS
    return temp;
}

/**
 * @brief Waits for the Data Ready (DRDY) pin to go low.
 *
 * Polls the DRDY pin until it becomes low, indicating that a new conversion
 * result is available. Includes a timeout to prevent indefinite waiting.
 */
static void ADS1256_WaitDRDY(void)
{
    UDOUBLE i = 0;
    const UDOUBLE timeout_cycles = 4000000; // Timeout threshold
    for(i = 0; i < timeout_cycles; i++) {
        if(DEV_Digital_Read(DEV_DRDY_PIN) == 0) {
            return; // DRDY is low, data ready
        }
    }
    // If loop finishes, DRDY timeout occurred
    Debug("ADS1256_WaitDRDY: Timeout! DRDY pin did not go low.\n");
    // Consider adding more robust error handling here, e.g., returning a status.
}

/**
 * @brief Reads the Chip ID from the ADS1256.
 *
 * The Chip ID is bits 7:4 of the STATUS register.
 * @return The 4-bit chip ID. Expected value is 3 for ADS1256.
 */
UBYTE ADS1256_ReadChipID(void)
{
    UBYTE id;
    // ADS1256_WaitDRDY(); // Not strictly necessary before reading a register unless a conversion is in progress
                         // and we need to ensure it's complete. For reading ID, it's generally safe.
    id = ADS1256_Read_data(REG_STATUS);
    return (id >> 4); // Chip ID is in the upper nibble of the STATUS register
}

/**
 * @brief Configures the ADS1256 ADC settings.
 *
 * Sets the gain, data rate, and other parameters in the ADC's registers.
 * @param gain The PGA gain setting (ADS1256_GAIN enum).
 * @param drate The data rate (ADS1256_DRATE enum).
 */
void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate)
{
    ADS1256_WaitDRDY(); // Wait for any ongoing conversion to complete before reconfiguring

    UBYTE status_reg = (0 << 7) | // ORDER: MSB first (default)
                       (0 << 6) | // ACAL: Auto-Calibration disabled (can be enabled if needed: 1 << 6)
                       (1 << 5) | // BUFEN: Analog input buffer enabled (set to 0 if buffer disabled in original code was intentional)
                       (0 << 4) | // DRDY_MODE: DRDY pin reflects DRDY state (default)
                       (0 << 3) | // CLKOUT: Clock output disabled (default)
                       (0 << 2) | // SDCS: Sensor detect current sources off (default)
                       (0 << 0);  // Lower 2 bits are read-only status bits

    UBYTE mux_reg = 0x01; // MUX: AIN0 positive, AINCOM negative (default single-ended for channel 0)
                          // This will be changed by SetChannel/SetDiffChannel later.

    UBYTE adcon_reg = (0 << 5) | // CLK_OFF: Internal oscillator ON (default)
                      (0 << 3) | // SDCS_MODE: Sensor detect current sources controlled by SDCS bits in STATUS (default)
                      (gain & 0x07); // PGA: Gain setting (masked to 3 bits)

    UBYTE drate_reg = ADS1256_DRATE_E[drate]; // Data rate setting

    DEV_Digital_Write(DEV_CS_PIN, 0); // Assert CS
    DEV_SPI_WriteByte(CMD_WREG | REG_STATUS); // Start writing from STATUS register (address 0)
    DEV_SPI_WriteByte(0x03); // Write 4 registers (STATUS, MUX, ADCON, DRATE)

    DEV_SPI_WriteByte(status_reg);
    DEV_SPI_WriteByte(mux_reg);    // Default MUX, will be overwritten by channel selection
    DEV_SPI_WriteByte(adcon_reg);
    DEV_SPI_WriteByte(drate_reg);

    DEV_Digital_Write(DEV_CS_PIN, 1); // De-assert CS
    DEV_Delay_ms(1); // Allow time for configuration to take effect and for self-calibration if ACAL was enabled.
                     // If ACAL is enabled, a longer delay might be needed (see datasheet).
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
    // MUX register: PSEL3-0 = Channel, NSEL3-0 = AINCOM (0x8)
    ADS1256_WriteReg(REG_MUX, (Channel << 4) | 0x08);
}

/**
 * @brief Sets the active input channel pair for differential measurements.
 * @param Channel The differential channel pair index (0-3).
 *        0: AIN0(+)-AIN1(-)
 *        1: AIN2(+)-AIN3(-)
 *        2: AIN4(+)-AIN5(-)
 *        3: AIN6(+)-AIN7(-)
 */
static void ADS1256_SetDiffChannel(UBYTE Channel) // Renamed from SetDiffChannal for consistency
{
    UBYTE mux_val = 0;
    switch (Channel) {
        case 0: mux_val = (0x00 << 4) | 0x01; break; // AIN0 - AIN1
        case 1: mux_val = (0x02 << 4) | 0x03; break; // AIN2 - AIN3
        case 2: mux_val = (0x04 << 4) | 0x05; break; // AIN4 - AIN5
        case 3: mux_val = (0x06 << 4) | 0x07; break; // AIN6 - AIN7
        default:
            Debug("ADS1256_SetDiffChannel: Invalid differential channel %d\n", Channel);
            return;
    }
    ADS1256_WriteReg(REG_MUX, mux_val);
}

/**
 * @brief Sets the measurement mode (single-ended or differential).
 * @param Mode 0 for single-ended, 1 for differential.
 */
void ADS1256_SetMode(UBYTE Mode)
{
    if (Mode == 0) {
        ScanMode = 0; // Single-ended input mode
        Debug("ADS1256_SetMode: Single-ended mode selected.\n");
    } else if (Mode == 1) {
        ScanMode = 1; // Differential input mode
        Debug("ADS1256_SetMode: Differential mode selected.\n");
    } else {
        Debug("ADS1256_SetMode: Invalid mode %d. Defaulting to single-ended.\n", Mode);
        ScanMode = 0;
    }
}

/**
 * @brief Initializes the ADS1256 module.
 *
 * Performs a reset, checks the chip ID, and configures the ADC with default settings.
 * @return 0 on successful initialization, 1 on failure (e.g., incorrect chip ID).
 */
UBYTE ADS1256_init(void)
{
    ADS1256_reset();
    UBYTE chip_id = ADS1256_ReadChipID();
    if (chip_id == 3) { // ADS1256 chip ID is 3 (binary 0011)
        Debug("ADS1256_init: Chip ID read success (ID: %d)\r\n", chip_id);
    } else {
        fprintf(stderr, "ADS1256_init: Chip ID read failed (Expected: 3, Got: %d)\r\n", chip_id);
        return 1; // Initialization failed
    }
    // Configure ADC with default gain 1 and a common data rate (e.g., 30kSPS or a slower one for stability)
    // The original code used ADS1256_30000SPS. Consider if this is always the best default.
    ADS1256_ConfigADC(ADS1256_GAIN_1, ADS1256_15000SPS); // Example: 15kSPS. NOTE: ADS1256_DRATE_E is an array, not the enum itself for this func
    // ADS1256_WriteCmd(CMD_SELFCAL); // Perform self-calibration after initial config
    // DEV_Delay_ms(200); // Allow time for self-calibration
    // ADS1256_WaitDRDY(); // Wait for calibration to complete

    Debug("ADS1256_init: Initialization complete.\n");
    return 0; // Initialization successful
}

/**
 * @brief Reads the raw 24-bit ADC conversion data.
 *
 * Assumes DRDY has already gone low. Sends RDATA command and reads 3 bytes.
 * @return The raw 24-bit ADC data, sign-extended to UDOUBLE (uint32_t).
 */
static UDOUBLE ADS1256_read_ADC_Data(void)
{
    UDOUBLE read_value = 0;
    UBYTE buf[3] = {0, 0, 0};

    // ADS1256_WaitDRDY(); // This should be called *before* calling this function.
                          // The caller is responsible for ensuring data is ready.

    DEV_Digital_Write(DEV_CS_PIN, 0); // Assert CS
    DEV_SPI_WriteByte(CMD_RDATA);     // Send RDATA command
    // DEV_Delay_ms(1); // Small delay, t_6 in datasheet (SCLK falling to DRDY rising after RDATA)
                       // is 24 * t_CLKIN. For 7.68MHz, this is ~3.125us. Data is clocked out after this.
                       // A small delay might be beneficial for some SPI configurations.

    buf[0] = DEV_SPI_ReadByte(); // Read MSB
    buf[1] = DEV_SPI_ReadByte(); // Read middle byte
    buf[2] = DEV_SPI_ReadByte(); // Read LSB
    DEV_Digital_Write(DEV_CS_PIN, 1); // De-assert CS

    // Combine the bytes into a 24-bit value
    read_value = ((UDOUBLE)buf[0] << 16) | ((UDOUBLE)buf[1] << 8) | (UDOUBLE)buf[2];

    // Sign extension for 24-bit two's complement value
    // If the 24th bit (MSB of the 24-bit data) is 1, the number is negative.
    if (read_value & 0x00800000) {
        read_value |= 0xFF000000; // Extend the sign to 32 bits
    }

    return read_value;
}

/**
 * @brief Gets the ADC conversion value for a specified channel.
 *
 * Handles both single-ended and differential modes based on `ScanMode`.
 * Waits for DRDY, sets the channel, issues SYNC and WAKEUP commands, then reads data.
 * @param Channel The channel number (0-7 for single-ended, 0-3 for differential).
 * @return The raw ADC value for the channel, sign-extended.
 *         Returns 0 if an invalid channel is specified for the current mode.
 */
UDOUBLE ADS1256_GetChannalValue(UBYTE Channel) // Original spelling "Channal"
{
    UDOUBLE value = 0;

    if (ScanMode == 0) { // Single-ended input mode
        if (Channel >= 8) {
            Debug("ADS1256_GetChannalValue: Invalid single-ended channel %d\n", Channel);
            return 0;
        }
        ADS1256_SetChannel(Channel);
    } else { // Differential input mode
        if (Channel >= 4) {
            Debug("ADS1256_GetChannalValue: Invalid differential channel %d\n", Channel);
            return 0;
        }
        ADS1256_SetDiffChannel(Channel);
    }

    // After changing MUX, a SYNC and WAKEUP sequence is typically required
    // to start a new conversion on the selected channel.
    ADS1256_WriteCmd(CMD_SYNC);   // Synchronize ADC conversion
    // DEV_Delay_ms(1); // Short delay after SYNC, t_11 in datasheet (SYNC to DRDY low) depends on data rate.
    ADS1256_WriteCmd(CMD_WAKEUP); // Wake up ADC from standby (completes SYNC)
    // DEV_Delay_ms(1); // Short delay after WAKEUP.

    ADS1256_WaitDRDY(); // Wait for the conversion on the new channel to complete
    value = ADS1256_read_ADC_Data();

    return value;
}

/**
 * @brief Reads ADC values from all 8 (single-ended) or 4 (differential) channels.
 *
 * Iterates through the available channels based on `ScanMode` and populates
 * the `ADC_Value` array. Note: If in differential mode, it still attempts to read 8 times,
 * effectively reading each differential pair twice. This behavior matches the original code.
 * For true differential reading of 4 pairs, the loop should be `i < 4`.
 *
 * @param ADC_Value Pointer to an array where the ADC values will be stored.
 *                  The array should be large enough for 8 UDOUBLE values.
 */
void ADS1256_GetAll(UDOUBLE *ADC_Value)
{
    UBYTE i;
    // The loop iterates 8 times regardless of ScanMode. If ScanMode is differential (1),
    // ADS1256_GetChannalValue will return 0 for channels 4-7.
    // This matches the original code's behavior.
    // If the intent is to read only the valid channels for the mode:
    // UBYTE num_channels_to_read = (ScanMode == 0) ? 8 : 4;
    // for(i = 0; i < num_channels_to_read; i++) {
    for (i = 0; i < 8; i++) {
        ADC_Value[i] = ADS1256_GetChannalValue(i);
    }
}

// --- Performance Monitoring and Optimized Read Functions ---

/**
 * @brief Fixes sign extension for a raw 24-bit ADC value.
 * @param raw_value The raw 24-bit ADC value (potentially without correct 32-bit sign extension).
 * @return The sign-extended 32-bit value.
 */
static UDOUBLE ADS1256_fix_sign_extension(UDOUBLE raw_value) {
    if (raw_value & 0x00800000) { // Check 24th bit
        raw_value |= 0xFF000000;  // Extend sign to 32 bits
    }
    return raw_value;
}

/**
 * @brief Reads ADC data after ensuring proper settling time.
 *
 * Waits for multiple DRDY cycles (configurable, e.g., 5) after a channel change
 * or SYNC/WAKEUP to ensure the ADC output has settled.
 * This is crucial for accuracy, especially at higher data rates or after MUX changes.
 * @param num_settling_drdy_cycles Number of DRDY cycles to wait for settling. Recommended: 3-5.
 * @return Settled raw ADC data, sign-extended.
 */
static UDOUBLE ADS1256_read_ADC_Data_settled(UBYTE num_settling_drdy_cycles)
{
    UBYTE buf[3] = {0, 0, 0};
    UDOUBLE read_value = 0;

    if (num_settling_drdy_cycles == 0) num_settling_drdy_cycles = 1; // Ensure at least one DRDY wait

    for (UBYTE settle_count = 0; settle_count < num_settling_drdy_cycles; settle_count++) {
        ADS1256_WaitDRDY(); // Wait for DRDY to go low

        // Only perform the actual SPI read on the final DRDY cycle
        if (settle_count == num_settling_drdy_cycles - 1) {
            DEV_Digital_Write(DEV_CS_PIN, 0);
            DEV_SPI_WriteByte(CMD_RDATA);
            // Add small delay if needed based on t6 (see ADS1256_read_ADC_Data)
            buf[0] = DEV_SPI_ReadByte();
            buf[1] = DEV_SPI_ReadByte();
            buf[2] = DEV_SPI_ReadByte();
            DEV_Digital_Write(DEV_CS_PIN, 1);
        }
        // For all but the last DRDY cycle, we need to ensure DRDY goes high again
        // to correctly time the next DRDY falling edge. This is implicitly handled
        // if the next operation is another ADS1256_WaitDRDY().
        // If not reading data, ensure DRDY cycle completes by waiting for it to go high if necessary,
        // though simply waiting for the next DRDY low is usually sufficient.
    }

    read_value = ((UDOUBLE)buf[0] << 16) | ((UDOUBLE)buf[1] << 8) | (UDOUBLE)buf[2];
    return ADS1256_fix_sign_extension(read_value);
}

/**
 * @brief Optimized function to acquire data from a specified list of up to N single-ended channels.
 *
 * This function is designed for scenarios where a subset of channels needs to be read
 * with proper settling time for higher accuracy.
 * @param ADC_Value Pointer to an array to store the read ADC values.
 * @param channels Array of UBYTE specifying the channel numbers (0-7) to read.
 * @param num_channels_to_read The number of channels to read from the `channels` array (max 8).
 * @param settling_cycles Number of DRDY cycles to wait for settling after each channel switch.
 */
void ADS1256_GetNChannels_Optimized(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels_to_read, UBYTE settling_cycles)
{
    if (!ADC_Value || !channels || num_channels_to_read == 0) return;
    if (num_channels_to_read > 8) num_channels_to_read = 8; // Max 8 channels
    if (settling_cycles == 0) settling_cycles = 1; // Default to 1 settling cycle

    struct timeval scan_start_tv;
    gettimeofday(&scan_start_tv, NULL);

    for (UBYTE i = 0; i < num_channels_to_read; i++) {
        UBYTE current_channel = channels[i];

        if (current_channel > 7) { // Validate channel number for single-ended
            ADC_Value[i] = 0; // Or some error indicator
            Debug("ADS1256_GetNChannels_Optimized: Invalid channel %d requested.\n", current_channel);
            continue;
        }

        ADS1256_SetChannel(current_channel);

        ADS1256_WriteCmd(CMD_SYNC);
        ADS1256_WriteCmd(CMD_WAKEUP);

        ADC_Value[i] = ADS1256_read_ADC_Data_settled(settling_cycles);
    }

    // Update performance metrics
    perf_metrics.total_scans++; // This represents one full N-channel acquisition sequence.

    struct timeval current_time_tv;
    gettimeofday(&current_time_tv, NULL);
    double elapsed_seconds = (current_time_tv.tv_sec - perf_metrics.start_time.tv_sec) +
                           (current_time_tv.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;

    if (elapsed_seconds > 0 && perf_metrics.theoretical_max_per_channel > 0) {
        // Total samples taken in all scans so far for these N channels
        double total_samples_for_n_channels = perf_metrics.total_scans * num_channels_to_read;
        perf_metrics.actual_total = total_samples_for_n_channels / elapsed_seconds; // Overall SPS for the N-channel group
        perf_metrics.actual_per_channel = perf_metrics.actual_total / num_channels_to_read; // Average SPS per channel within the group

        perf_metrics.efficiency_percent = (perf_metrics.actual_per_channel / perf_metrics.theoretical_max_per_channel) * 100.0;
    }
}

/**
 * @brief Ultra-fast acquisition from a list of up to N single-ended channels with minimal overhead.
 *
 * This function prioritizes speed over accuracy by reducing settling times and combining SPI transactions where feasible.
 * @param ADC_Value Pointer to an array to store the read ADC values.
 * @param channels Array of UBYTE specifying the channel numbers (0-7) to read.
 * @param num_channels_to_read The number of channels to read from the `channels` array (max 8).
 */
void ADS1256_GetNChannels_Fast(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels_to_read)
{
    if (!ADC_Value || !channels || num_channels_to_read == 0) return;
    if (num_channels_to_read > 8) num_channels_to_read = 8;

    for (UBYTE i = 0; i < num_channels_to_read; i++) {
        UBYTE current_channel = channels[i];

        if (current_channel > 7) {
            ADC_Value[i] = 0;
            Debug("ADS1256_GetNChannels_Fast: Invalid channel %d requested.\n", current_channel);
            continue;
        }

        DEV_Digital_Write(DEV_CS_PIN, 0);
        DEV_SPI_WriteByte(CMD_WREG | REG_MUX);      // WREG MUX
        DEV_SPI_WriteByte(0x00);                    // Write 1 register
        DEV_SPI_WriteByte((current_channel << 4) | 0x08); // Channel to AINCOM
        DEV_Digital_Write(DEV_CS_PIN, 1);
        // Minimal delay if any, e.g., DEV_Delay_us(1);

        ADS1256_WriteCmd(CMD_SYNC);
        ADS1256_WriteCmd(CMD_WAKEUP);

        // Quick settling - wait for minimal DRDY cycles (e.g., 1)
        ADS1256_WaitDRDY();

        ADC_Value[i] = ADS1256_read_ADC_Data();
    }
    perf_metrics.total_scans++; // Update scan count (one N-channel acquisition)
}


/**
 * @brief Initializes performance monitoring for N-channel acquisition.
 * @param drate_enum_val The configured data rate (ADS1256_DRATE enum value) which determines
 *                       the theoretical maximum samples per second for a single, continuously read channel.
 */
void ADS1256_InitPerformanceMonitoring(ADS1256_DRATE drate_enum_val)
{
    float sps_value_from_drate_enum = 0.0f;
    // This lookup should map the enum to the SPS value from the datasheet (Table 3)
    switch(drate_enum_val) {
        case ADS1256_30000SPS: sps_value_from_drate_enum = 30000.0f; break;
        case ADS1256_15000SPS: sps_value_from_drate_enum = 15000.0f; break;
        case ADS1256_7500SPS:  sps_value_from_drate_enum = 7500.0f;  break;
        case ADS1256_3750SPS:  sps_value_from_drate_enum = 3750.0f;  break;
        case ADS1256_2000SPS:  sps_value_from_drate_enum = 2000.0f;  break;
        case ADS1256_1000SPS:  sps_value_from_drate_enum = 1000.0f;  break;
        case ADS1256_500SPS:   sps_value_from_drate_enum = 500.0f;   break;
        case ADS1256_100SPS:   sps_value_from_drate_enum = 100.0f;   break;
        case ADS1256_60SPS:    sps_value_from_drate_enum = 60.0f;    break;
        case ADS1256_50SPS:    sps_value_from_drate_enum = 50.0f;    break;
        case ADS1256_30SPS:    sps_value_from_drate_enum = 30.0f;    break;
        case ADS1256_25SPS:    sps_value_from_drate_enum = 25.0f;    break;
        case ADS1256_15SPS:    sps_value_from_drate_enum = 15.0f;    break;
        case ADS1256_10SPS:    sps_value_from_drate_enum = 10.0f;    break;
        case ADS1256_5SPS:     sps_value_from_drate_enum = 5.0f;     break;
        case ADS1256_2d5SPS:   sps_value_from_drate_enum = 2.5f;     break;
        default: sps_value_from_drate_enum = 2.5f; // Fallback to lowest SPS
                 Debug("ADS1256_InitPerformanceMonitoring: Unknown DRATE enum value, defaulting to 2.5 SPS.\n");
    }

    perf_metrics.theoretical_max_per_channel = sps_value_from_drate_enum;
    // theoretical_total is more complex. If N channels are multiplexed, the total SPS will be less than N * theoretical_max_per_channel
    // due to MUX switching, SYNC, WAKEUP, and settling times. For now, we can set it based on single channel for reference.
    perf_metrics.theoretical_total = sps_value_from_drate_enum; // This is the max for ONE channel continuously.
                                                              // Actual total for N channels will be lower.
    perf_metrics.actual_per_channel = 0;
    perf_metrics.actual_total = 0;
    perf_metrics.efficiency_percent = 0;
    perf_metrics.total_scans = 0;

    gettimeofday(&perf_metrics.start_time, NULL);

    Debug("=== ADS1256 Performance Monitor Initialized ===\n");
    Debug("Theoretical Max (single channel continuous): %.0f SPS\n",
           perf_metrics.theoretical_max_per_channel);
    Debug("Actual rates will be calculated based on N-channel scan operations.\n\n");
}

/**
 * @brief Gets a pointer to the current performance metrics structure.
 * @return Pointer to the static `perf_metrics` structure.
 */
performance_metrics_t* ADS1256_GetPerformanceMetrics(void)
{
    return &perf_metrics;
}

/**
 * @brief Prints a detailed performance report to stdout.
 */
void ADS1256_PrintPerformanceReport(void)
{
    struct timeval current_time_tv;
    gettimeofday(&current_time_tv, NULL);
    double elapsed_seconds = (current_time_tv.tv_sec - perf_metrics.start_time.tv_sec) +
                           (current_time_tv.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;

    printf("\n=== ADS1256 Performance Report ===\n");
    printf("Runtime: %.2f seconds\n", elapsed_seconds);
    printf("Total N-Channel Scan Operations (total_scans): %lu\n", perf_metrics.total_scans);
    printf("Theoretical Max (single channel continuous): %.0f SPS\n",
           perf_metrics.theoretical_max_per_channel);
    printf("Actual Average Total SPS (all scanned channels combined): %.1f SPS\n",
           perf_metrics.actual_total); // actual_total should be updated by scan functions
    printf("Actual Average Per-Channel SPS (when scanning N channels): %.1f SPS\n",
           perf_metrics.actual_per_channel); // actual_per_channel should be updated by scan functions

    if (perf_metrics.total_scans > 0 && perf_metrics.actual_per_channel > 0 && perf_metrics.theoretical_max_per_channel > 0) {
        printf("Efficiency (Actual Per-Channel SPS vs Theoretical Single Channel Max): %.1f%%\n", perf_metrics.efficiency_percent);
    } else {
        printf("Efficiency: N/A (insufficient data or zero theoretical max)\n");
    }

    if (perf_metrics.efficiency_percent > 90) {
        printf("Status: EXCELLENT - Near theoretical limits for the operation type.\n");
    } else if (perf_metrics.efficiency_percent > 75) {
        printf("Status: GOOD - Acceptable performance.\n");
    } else if (perf_metrics.efficiency_percent > 50) {
        printf("Status: FAIR - Consider optimization if higher throughput needed.\n");
    } else if (perf_metrics.total_scans > 0) {
        printf("Status: POOR - Significant optimization may be needed or expectations reviewed.\n");
    } else {
        printf("Status: No scan data yet.\n");
    }
    printf("=============================================\n\n");
}

/**
 * @brief Calculates and returns the current efficiency percentage.
 * @return Current efficiency, or 0.0 if not enough data.
 */
double ADS1256_GetCurrentEfficiency(void)
{
    return perf_metrics.efficiency_percent;
}

/**
 * @brief Checks if performance is meeting a 'good' threshold (e.g., >75% efficiency).
 * @return 1 if performance is good, 0 otherwise.
 */
int ADS1256_IsPerformanceGood(void)
{
    return (perf_metrics.efficiency_percent > 75.0 && perf_metrics.total_scans > 10);
}

/**
 * @brief Converts a raw ADC value to voltage.
 *
 * Assumes VREF is 5.0V. The PGA gain setting must be accounted for by the caller
 * if it's not 1. The ADS1256 outputs 24-bit two's complement data.
 *  +VREF (FS) corresponds to 0x7FFFFF (8388607 decimal).
 *  -VREF (-FS) corresponds to 0x800000 (sign-extended to 0xFF800000 for int32_t, or -8388608 decimal).
 *  0V corresponds to 0x000000.
 *
 * @param raw_value The raw 24-bit ADC code, sign-extended and stored in a UDOUBLE (uint32_t).
 * @param vref The reference voltage used by the ADC (e.g., 5.0V).
 * @param gain_enum The PGA gain setting used during the ADC conversion (ADS1256_GAIN enum value).
 * @return The calculated voltage as a float.
 */
float ADS1256_RawToVoltage(UDOUBLE raw_value, float vref, ADS1256_GAIN gain_enum)
{
    int32_t signed_adc_code = (int32_t)raw_value;
    float pga_gain_val = 1.0f;

    switch(gain_enum) {
        case ADS1256_GAIN_1  : pga_gain_val = 1.0f; break;
        case ADS1256_GAIN_2  : pga_gain_val = 2.0f; break;
        case ADS1256_GAIN_4  : pga_gain_val = 4.0f; break;
        case ADS1256_GAIN_8  : pga_gain_val = 8.0f; break;
        case ADS1256_GAIN_16 : pga_gain_val = 16.0f; break;
        case ADS1256_GAIN_32 : pga_gain_val = 32.0f; break;
        case ADS1256_GAIN_64 : pga_gain_val = 64.0f; break;
        // default: Debug("ADS1256_RawToVoltage: Unknown gain enum %d, using gain 1.\n", gain_enum); pga_gain_val = 1.0f; break; // Optional: Add debug for unknown gain
    }

    // Voltage = (ADC_code / (2^23 - 1)) * (VREF / PGA_GAIN)
    // (2^23 - 1) is 8388607.0 for positive full scale.
    // For bipolar, the range is -2^23 to (2^23 - 1).
    // So, for a code C, voltage = (C / 2^23) * (VREF / PGA_GAIN) is a common approximation for bipolar when C is signed.
    // Or, more precisely for 2s complement: V = (signed_code / MAX_CODE_POSITIVE) * (VREF_EFFECTIVE_RANGE / 2) / PGA_GAIN
    // Given the datasheet formula V_IN = (CODE / 2^23) * V_REF / PGA (for bipolar mode, where CODE can be negative)
    // Max positive code 0x7FFFFF = 8388607. Min negative code 0x800000 = -8388608 (as int32_t)
    // So, the scaling factor should be based on 2^23 = 8388608.0f

    return ((float)signed_adc_code / 8388608.0f) * (vref / pga_gain_val);
}
