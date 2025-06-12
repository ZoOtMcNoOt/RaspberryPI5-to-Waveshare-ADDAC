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

#include "../../common/DEV_Config.h" 
#include <sys/time.h>              

#define ADS1256_ID (0x03) ///< Expected Chip ID for ADS1256

/** @brief Number of single-ended input channels (AIN0-AIN7) */
#define NUM_SINGLE_ENDED_CHANNELS 8
/** @brief Number of differential input pairs (AIN0-1, AIN2-3, AIN4-5, AIN6-7) */
#define NUM_DIFFERENTIAL_PAIRS 4

/** @brief Default positive reference voltage (e.g., from an external +5V supply) */
#define ADC_VREF_POS_5V0 5.0f
/** @brief Default negative reference voltage (e.g., connected to AGND) */
#define ADC_VREF_NEG_GND 0.0f

/**
 * @brief Enumeration for ADC scan modes.
 */
typedef enum {
    SCAN_MODE_SINGLE_ENDED = 0,         ///< Single-ended input mode (AINx vs AINCOM)
    SCAN_MODE_DIFFERENTIAL_INPUTS = 1   ///< Differential input mode (AINx vs AINy)
} ADS1256_SCAN_MODE;

/**
 * @brief Enumeration for ADC gain settings (PGA).
 */
typedef enum
{
    ADS1256_GAIN_1   = 0, ///< Gain 1 (Input range: +- 5 V)
    ADS1256_GAIN_2   = 1, ///< Gain 2 (Input range: +- 2.5 V)
    ADS1256_GAIN_4   = 2, ///< Gain 4 (Input range: +- 1.25 V)
    ADS1256_GAIN_8   = 3, ///< Gain 8 (Input range: +- 0.625 V)
    ADS1256_GAIN_16  = 4, ///< Gain 16 (Input range: +- 0.3125 V)
    ADS1256_GAIN_32  = 5, ///< Gain 32 (Input range: +- 0.15625 V)
    ADS1256_GAIN_64  = 6, ///< Gain 64 (Input range: +- 0.078125 V)
} ADS1256_GAIN;

/**
 * @brief Enumeration for ADC data rates (SPS - Samples Per Second).
 *  Set a data rate of a programmable filter (programmable averager).
 * Programmable from 30,000 to 2.5 samples per second (SPS).
 * Setting the data rate to high value results in smaller resolution of the data.
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
    ADS1256_2d5SPS,       ///< 2.5 SPS 
    ADS1256_DRATE_MAX     ///< Sentinel for the number of data rates
} ADS1256_DRATE;

/**
 * @brief Enumeration for ADS1256 register addresses.
 * Set of registers.
 * The operation of the ADS1256 is controlled through a set of registers.
 * Collectively, the registers contain all the information needed to configure
 * data rate, multiplexer settings, PGA setting, calibration, etc.
 */
typedef enum
{
    REG_STATUS = 0, // Status Register
                    // Register address: 00h, Reset value: x1H

    REG_MUX    = 1, // Multiplexer Control Register
                    // Register address: 01h, Reset value: 01H

    REG_ADCON  = 2, // A/D Control Register
                    // Register address: 02h, Reset value: 20H

    REG_DRATE  = 3, // Data Rate Register
                    // Register address: 03h, Reset value: F0H

    REG_IO     = 4, // GPIO Control Register
                    // Register address: 04h, Reset value: E0H

    REG_OFC0   = 5, // Offset Calibration Coefficient Byte 0
                    // Register address: 05h, Reset value: xxH

    REG_OFC1   = 6, // Offset Calibration Coefficient Byte 1
                    // Register address: 06h, Reset value: xxH

    REG_OFC2   = 7, // Offset Calibration Coefficient Byte 2
                    // Register address: 07h, Reset value: xxH

    REG_FSC0   = 8, // Full-Scale Calibration Coefficient Byte 0
                    // Register address: 08h, Reset value: xxH

    REG_FSC1   = 9, // Full-Scale Calibration Coefficient Byte 1
                    // Register address: 09h, Reset value: xxH

    REG_FSC2   = 10,// Full-Scale Calibration Coefficient Byte 2
                    // Register address: 0Ah, Reset value: xxH
} ADS1256_REG;

/**
 * @brief Enumeration for ADS1256 MUX register channel selection values.
 * Used for PSEL and NSEL fields in the MUX register.
 */
typedef enum {
    ADS1256_MUX_AIN0    = 0x0, ///< AIN0
    ADS1256_MUX_AIN1    = 0x1, ///< AIN1
    ADS1256_MUX_AIN2    = 0x2, ///< AIN2
    ADS1256_MUX_AIN3    = 0x3, ///< AIN3
    ADS1256_MUX_AIN4    = 0x4, ///< AIN4
    ADS1256_MUX_AIN5    = 0x5, ///< AIN5
    ADS1256_MUX_AIN6    = 0x6, ///< AIN6
    ADS1256_MUX_AIN7    = 0x7, ///< AIN7
    ADS1256_MUX_AINCOM  = 0x8, ///< AINCOM (Analog Common)
    // Values 0x9 to 0xF are reserved or have special meanings (e.g., test voltages)
} ADS1256_MUX_CHANNEL;

/**
 * @brief Enumeration for ADS1256 commands.
 */
typedef enum
{
    CMD_WAKEUP   = 0x00, ///< Completes SYNC and Exits Standby Mode
    CMD_RDATA    = 0x01, ///< Read Data
    CMD_RDATAC   = 0x03, ///< Read Data Continuously
    CMD_SDATAC   = 0x0F, ///< Stop Read Data Continuously
    CMD_RREG     = 0x10, ///< Read from REG - 1st command byte: 0001rrrr 
						 //					  2nd command byte: 0000nnnn
    CMD_WREG     = 0x50, ///< Write to REG  - 1st command byte: 0001rrrr
						 //					  2nd command byte: 0000nnnn
                         // r = starting reg address, n = number of reg addresses
    CMD_SELFCAL  = 0xF0, ///< Offset and Gain Self-Calibration
    CMD_SELFOCAL = 0xF1, ///< Offset Self-Calibration
    CMD_SELFGCAL = 0xF2, ///< Gain Self-Calibration
    CMD_SYSOCAL  = 0xF3, ///< System Offset Calibration
    CMD_SYSGCAL  = 0xF4, ///< System Gain Calibration
    CMD_SYNC     = 0xFC, ///< Synchronize the A/D Conversion
    CMD_STANDBY  = 0xFD, ///< Begin Standby Mode 
    CMD_RESET    = 0xFE, ///< Reset to Power-Up Values
} ADS1256_CMD;

/**
 * @brief Lookup table for DRATE register values corresponding to ADS1256_DRATE enum.
 */
static const uint8_t ADS1256_DRATE_E[ADS1256_DRATE_MAX] =
{
    0xF0, ///< 30000SPS
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
    0x23, ///< 10SPS 
    0x13, ///< 5SPS
    0x03  ///< 2.5SPS
};

/**
 * @brief Structure to hold performance metrics for ADC operations.
 */
typedef struct {
    double theoretical_sps_per_channel; ///< Theoretical max SPS for one channel at current DRATE.
    double actual_avg_sps_per_channel;  ///< Actual measured average SPS per channel when scanning N channels.
    double actual_avg_sps_total;        ///< Actual total measured SPS (sum of all samples from all scanned channels / time).
    double efficiency_percent;          ///< Efficiency: (actual_avg_sps_per_channel / theoretical_sps_per_channel) * 100.
    unsigned long total_samples_acquired; ///< Total individual samples acquired since monitoring started.
    unsigned long total_n_channel_scans;  ///< Total number of N-channel scan operations performed.
    struct timeval start_time;          ///< Timestamp when performance monitoring started.
} performance_metrics_t;

/*--------------------------------------------------------------------------
                            Function Prototypes
---------------------------------------------------------------------------*/
/**
 * @brief Initializes the ADS1256 module.
 * @param drate The desired data rate (ADS1256_DRATE enum).
 * @param gain The desired PGA gain (ADS1256_GAIN enum).
 * @param scan_mode The desired scan mode (SCAN_MODE_SINGLE_ENDED or SCAN_MODE_DIFFERENTIAL_INPUTS).
 * @return 0 on success, 1 on failure.
 */
UBYTE ADS1256_init(ADS1256_DRATE drate, ADS1256_GAIN gain, ADS1256_SCAN_MODE scan_mode);

/**
 * @brief Configures the ADC gain and data rate.
 * @param gain The desired gain (ADS1256_GAIN enum).
 * @param drate The desired data rate (ADS1256_DRATE enum).
 */
void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate);

/**
 * @brief Reads the ADC value for a single specified channel.
 * @param Channel The channel number (0-7 for single-ended, 0-3 for differential pair index).
 * @return Raw 24-bit ADC value, sign-extended.
 */
UDOUBLE ADS1256_GetChannelValue(UBYTE Channel);

/**
 * @brief Reads all relevant channels based on ScanMode and populates the provided array.
 * @param ADC_Value Pointer to an array to store ADC values.
 *                  Size should be NUM_SINGLE_ENDED_CHANNELS or NUM_DIFFERENTIAL_PAIRS.
 */
void ADS1256_GetAllChannels(UDOUBLE *ADC_Value);

/**
 * @brief Reads the 4-bit chip ID from the ADS1256.
 * @return Chip ID (expected to be ADS1256_ID).
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
 * @brief Acquires data from a list of N single-ended channels with minimal overhead.
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

// === Utility Functions ===
/**
 * @brief Converts a raw ADC value (24-bit two's complement) to voltage.
 * @param raw_value The raw ADC data (sign-extended UDOUBLE).
 * @param vref_positive Positive reference voltage (e.g., 5.0V).
 * @param vref_negative Negative reference voltage (e.g., 0.0V for GND).
 * @param gain_enum The PGA gain setting (ADS1256_GAIN enum) used for the conversion.
 * @return The calculated voltage as a float.
 */
float ADS1256_RawToVoltage(UDOUBLE raw_value, float vref_positive, float vref_negative, ADS1256_GAIN gain_enum);

#endif // _ADS1256_H_