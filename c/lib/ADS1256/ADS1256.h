/**
 * @file ADS1256.h
 * @brief Header file for the ADS1256 24-bit Analog-to-Digital Converter driver.
 *
 * Defines types, enums for configuration (gain, data rate, registers, commands),
 * the ADS1256_DRATE_E lookup table, the performance metrics structure,
 * and function prototypes for interacting with the ADS1256 ADC.
 */

#ifndef _ADS1256_H_
#define _ADS1256_H_

#include "../../common/DEV_Config.h" // For UBYTE, UWORD, UDOUBLE, and hardware interface macros/functions
#include <sys/time.h>               // For struct timeval (used in performance_metrics_t)

/**
 * @brief Enumeration for ADC gain settings (PGA).
 */
typedef enum
{
    ADS1256_GAIN_1   = 0, ///< Gain 1
    ADS1256_GAIN_2   = 1, ///< Gain 2
    ADS1256_GAIN_4   = 2, ///< Gain 4
    ADS1256_GAIN_8   = 3, ///< Gain 8
    ADS1256_GAIN_16  = 4, ///< Gain 16
    ADS1256_GAIN_32  = 5, ///< Gain 32
    ADS1256_GAIN_64  = 6, ///< Gain 64
    // ADS1256_GAIN_MAX // Optional: for bounds checking
} ADS1256_GAIN;

/**
 * @brief Enumeration for ADC data rates (SPS - Samples Per Second).
 */
typedef enum
{
    ADS1256_30000SPS = 0, ///< 30,000 SPS
    ADS1256_15000SPS,     ///< 15,000 SPS
    ADS1256_7500SPS,      ///< 7,500 SPS
    ADS1256_3750SPS,      ///< 3,750 SPS
    ADS1256_2000SPS,      ///< 2,000 SPS
    ADS1256_1000SPS,      ///< 1,000 SPS
    ADS1256_500SPS,       ///< 500 SPS
    ADS1256_100SPS,       ///< 100 SPS
    ADS1256_60SPS,        ///< 60 SPS
    ADS1256_50SPS,        ///< 50 SPS
    ADS1256_30SPS,        ///< 30 SPS
    ADS1256_25SPS,        ///< 25 SPS
    ADS1256_15SPS,        ///< 15 SPS
    ADS1256_10SPS,        ///< 10 SPS
    ADS1256_5SPS,         ///< 5 SPS
    ADS1256_2d5SPS,       ///< 2.5 SPS (Note: 'd' for decimal point representation)
    
    ADS1256_DRATE_MAX     ///< Sentinel for the number of data rates
} ADS1256_DRATE;

/**
 * @brief Enumeration for ADS1256 register addresses.
 * Values correspond to the register map in the datasheet.
 */
typedef enum
{
    REG_STATUS = 0, ///< Status Register (Bits 7:4 ID, 3 BUFEN, 2 ACAL, 1 ORDER, 0 DRDY_MODE (ADS1255 only)) - Default 0x00 (ADS1256)
    REG_MUX    = 1, ///< Multiplexer Control Register (PSEL3-0, NSEL3-0) - Default 0x01
    REG_ADCON  = 2, ///< A/D Control Register (CLK1-0, SDCS1-0, PGA2-0) - Default 0x20
    REG_DRATE  = 3, ///< Data Rate Register (DR7-0) - Default 0xF0
    REG_IO     = 4, ///< GPIO Control Register (DIR3-0, DIO3-0) - Default 0xE0 (ADS1256 specific, not on ADS1255)
    REG_OFC0   = 5, ///< Offset Calibration Coefficient Byte 0 (MSB) - Default 0xXX
    REG_OFC1   = 6, ///< Offset Calibration Coefficient Byte 1 - Default 0xXX
    REG_OFC2   = 7, ///< Offset Calibration Coefficient Byte 2 (LSB) - Default 0xXX
    REG_FSC0   = 8, ///< Full-Scale Calibration Coefficient Byte 0 (MSB) - Default 0xXX
    REG_FSC1   = 9, ///< Full-Scale Calibration Coefficient Byte 1 - Default 0xXX
    REG_FSC2   = 10,///< Full-Scale Calibration Coefficient Byte 2 (LSB) - Default 0xXX
    // REG_MAX // Optional: for bounds checking
} ADS1256_REG;

/**
 * @brief Enumeration for ADS1256 commands.
 * Values correspond to the command definitions in the datasheet.
 */
typedef enum
{
    CMD_WAKEUP   = 0x00, ///< Completes SYNC and Exits Standby Mode
    CMD_RDATA    = 0x01, ///< Read Data
    CMD_RDATAC   = 0x03, ///< Read Data Continuously (not typically used in polled mode)
    CMD_SDATAC   = 0x0F, ///< Stop Read Data Continuously
    CMD_RREG     = 0x10, ///< Read from REG rrr (command format: 0001 rrrr)
    CMD_WREG     = 0x50, ///< Write to REG rrr (command format: 0101 rrrr)
    CMD_SELFCAL  = 0xF0, ///< Offset and Gain Self-Calibration
    CMD_SELFOCAL = 0xF1, ///< Offset Self-Calibration
    CMD_SELFGCAL = 0xF2, ///< Gain Self-Calibration
    CMD_SYSOCAL  = 0xF3, ///< System Offset Calibration
    CMD_SYSGCAL  = 0xF4, ///< System Gain Calibration
    CMD_SYNC     = 0xFC, ///< Synchronize the A/D Conversion (or begin standby)
    CMD_STANDBY  = 0xFD, ///< Begin Standby Mode (deprecated, use SYNC followed by WAKEUP)
    CMD_RESET    = 0xFE, ///< Reset to Power-Up Values
    // CMD_MAX // Optional: for bounds checking
} ADS1256_CMD;

/**
 * @brief Lookup table for DRATE register values corresponding to ADS1256_DRATE enum.
 * Index this array with an ADS1256_DRATE enum value to get the byte for the DRATE register.
 */
static const uint8_t ADS1256_DRATE_E[ADS1256_DRATE_MAX] =
{
    0xF0, ///< 30000SPS (default)
    0xE0, ///< 15000SPS
    0xD0, ///< 7500SPS
    0xC0, ///< 3750SPS
    0xB0, ///< 2000SPS
    0xA1, ///< 1000SPS
    0x92, ///< 500SPS
    0x82, ///< 100SPS
    0x72, ///< 60SPS
    0x63, ///< 50SPS
    0x53, ///< 30SPS
    0x43, ///< 25SPS
    0x33, ///< 15SPS
    0x23, ///< 10SPS (Note: datasheet says 0x23 for 10SPS, original code had 0x20 which is not in datasheet table)
    0x13, ///< 5SPS
    0x03  ///< 2.5SPS
};

/**
 * @brief Structure to hold performance metrics for ADC operations.
 */
typedef struct {
    double theoretical_max_per_channel; ///< Theoretical maximum SPS for a single channel at current DRATE.
    double theoretical_total;           ///< Theoretical total SPS (e.g., if all channels could be read at max rate, often same as per_channel for reference).
    double actual_per_channel;          ///< Actual measured SPS per channel (averaged over N channels if N are scanned).
    double actual_total;                ///< Actual total measured SPS (sum of samples from all scanned channels per second).
    double efficiency_percent;          ///< Efficiency: (actual_per_channel / theoretical_max_per_channel) * 100.
    unsigned long total_scans;          ///< Total number of N-channel scan operations performed.
    struct timeval start_time;          ///< Timestamp when performance monitoring started.
} performance_metrics_t;

/*--------------------------------------------------------------------------
                            Function Prototypes
---------------------------------------------------------------------------*/
/**
 * @brief Initializes the ADS1256 module.
 * @return 0 on success, 1 on failure.
 */
UBYTE ADS1256_init(void);

/**
 * @brief Sets the ADC scan mode (single-ended or differential).
 * @param Mode 0 for single-ended, 1 for differential.
 */
void ADS1256_SetMode(UBYTE Mode);

/**
 * @brief Configures the ADC gain and data rate.
 * @param gain The desired gain (ADS1256_GAIN enum).
 * @param drate The desired data rate (ADS1256_DRATE enum).
 */
void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate);

/**
 * @brief Reads the ADC value for a single specified channel.
 *        Considers the current ScanMode (single-ended or differential).
 * @param Channel The channel number (0-7 for single-ended, 0-3 for differential).
 * @return Raw 24-bit ADC value, sign-extended.
 */
UDOUBLE ADS1256_GetChannalValue(UBYTE Channel); // Retaining original spelling "Channal" for compatibility

/**
 * @brief Reads all 8 channels (if single-ended) or 4 differential pairs (effectively 8 reads).
 *        Populates the provided array with ADC values.
 * @param ADC_Value Pointer to an array (size 8) to store UDOUBLE ADC values.
 */
void ADS1256_GetAll(UDOUBLE *ADC_Value);

/**
 * @brief Reads the 4-bit chip ID from the ADS1256.
 * @return Chip ID (expected to be 3 for ADS1256).
 */
UBYTE ADS1256_ReadChipID(void);

// === Optimized N-Channel Read Functions ===
/**
 * @brief Acquires data from a specified list of N single-ended channels with proper settling.
 * @param ADC_Value Pointer to an array to store results.
 * @param channels Array of channel numbers (0-7) to read.
 * @param num_channels_to_read Number of channels in the `channels` array.
 * @param settling_cycles Number of DRDY cycles to wait for settling after each channel switch.
 */
void ADS1256_GetNChannels_Optimized(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels_to_read, UBYTE settling_cycles);

/**
 * @brief Acquires data from a list of N single-ended channels with minimal overhead (faster, less accurate).
 * @param ADC_Value Pointer to an array to store results.
 * @param channels Array of channel numbers (0-7) to read.
 * @param num_channels_to_read Number of channels in the `channels` array.
 */
void ADS1256_GetNChannels_Fast(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels_to_read);

// === Performance Monitoring Functions ===
/**
 * @brief Initializes performance monitoring based on the configured data rate.
 * @param drate_enum_val The ADS1256_DRATE enum value corresponding to the configured SPS.
 */
void ADS1256_InitPerformanceMonitoring(ADS1256_DRATE drate_enum_val);

/**
 * @brief Retrieves a pointer to the current performance metrics data.
 * @return Pointer to the `performance_metrics_t` structure.
 */
performance_metrics_t* ADS1256_GetPerformanceMetrics(void);

/**
 * @brief Prints a formatted performance report to standard output.
 */
void ADS1256_PrintPerformanceReport(void);

/**
 * @brief Gets the current calculated efficiency of ADC operations.
 * @return Efficiency percentage.
 */
double ADS1256_GetCurrentEfficiency(void);

/**
 * @brief Checks if the current performance is considered "good" (e.g., efficiency > 75%).
 * @return 1 if good, 0 otherwise.
 */
int ADS1256_IsPerformanceGood(void);

// === Utility Functions ===
/**
 * @brief Converts a raw ADC value (24-bit two's complement) to voltage.
 * @param raw_value The raw ADC data (sign-extended UDOUBLE).
 * @param vref The reference voltage (e.g., 5.0V).
 * @param gain_enum The PGA gain setting (ADS1256_GAIN enum) used for the conversion.
 * @return The calculated voltage as a float.
 */
float ADS1256_RawToVoltage(UDOUBLE raw_value, float vref, ADS1256_GAIN gain_enum);

#endif // _ADS1256_H_