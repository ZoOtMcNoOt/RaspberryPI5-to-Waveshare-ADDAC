#ifndef ADS1256_H
#define ADS1256_H

#include <stdint.h>

//####################################################################################
//
//##    1. ENUMERATIONS & TYPE DEFINITIONS
//
//####################################################################################

// Boolean type for clarity in function signatures
typedef enum {
    ADC_FALSE = 0,
    ADC_TRUE  = 1,
} adc_bool_t;

// Programmable Gain Amplifier (PGA) settings
typedef enum {
	PGA_GAIN_1   = 0, // +-5.0V
	PGA_GAIN_2   = 1, // +-2.5V
	PGA_GAIN_4   = 2, // +-1.25V
	PGA_GAIN_8   = 3, // +-0.625V
	PGA_GAIN_16  = 4, // +-0.3125V
	PGA_GAIN_32  = 5, // +-0.15625V
	PGA_GAIN_64  = 6  // +-0.078125V
} ads1256_pga_t;

// Data Rate settings in Samples Per Second (SPS)
typedef enum {
	DRATE_30000SPS = 0xF0, DRATE_15000SPS = 0xE0, DRATE_7500SPS  = 0xD0,
	DRATE_3750SPS  = 0xC0, DRATE_2000SPS  = 0xB0, DRATE_1000SPS  = 0xA1,
	DRATE_500SPS   = 0x92, DRATE_100SPS   = 0x82, DRATE_60SPS    = 0x72,
	DRATE_50SPS    = 0x63, DRATE_30SPS    = 0x53, DRATE_25SPS    = 0x43,
	DRATE_15SPS    = 0x33, DRATE_10SPS    = 0x20, DRATE_5SPS     = 0x13,
	DRATE_2D5SPS   = 0x03
} ads1256_drate_t;

// Analog Input Channels
typedef enum {
	AIN0 = 0, AIN1 = 1, AIN2 = 2, AIN3 = 3, AIN4 = 4, AIN5 = 5,
	AIN6 = 6, AIN7 = 7, AINCOM = 8
} ads1256_ain_t;


//####################################################################################
//
//##    2. PUBLIC FUNCTION DECLARATIONS
//
//####################################################################################

/**
 * @brief Initializes GPIO and SPI for communication with the ADS1256.
 * Must be called before any other driver function.
 * @return 1 on success, -1 on failure.
 */
int ads1256_init(void);

/**
 * @brief Releases GPIO and SPI resources.
 * Should be called at the end of the program to clean up.
 */
void ads1256_exit(void);

/**
 * @brief Configures the Programmable Gain Amplifier (PGA).
 * @param gain The desired gain setting from the ads1256_pga_t enum.
 */
void ads1256_set_pga(ads1256_pga_t gain);

/**
 * @brief Configures the data rate (samples per second).
 * @param drate The desired data rate from the ads1256_drate_t enum.
 */
void ads1256_set_drate(ads1256_drate_t drate);

/**
 * @brief Enables or disables the internal input buffer.
 * @param enable Set to ADC_TRUE to enable, ADC_FALSE to disable.
 */
void ads1256_set_buffer(adc_bool_t enable);

/**
 * @brief Performs a self-calibration of offset and gain.
 * This should be called after setting gain and data rate.
 */
void ads1256_calibrate(void);

/**
 * @brief Reads a single value from one single-ended channel.
 * This function handles setting the channel, synchronizing, and reading the data.
 * @param channel The analog input channel to read from (e.g., AIN0).
 * @return The 24-bit signed conversion result.
 */
int32_t ads1256_read_single_ended(ads1256_ain_t channel);

/**
 * @brief Reads a single value from one differential channel pair.
 * @param pos_ch The positive analog input channel (e.g., AIN0).
 * @param neg_ch The negative analog input channel (e.g., AIN1).
 * @return The 24-bit signed conversion result.
 */
int32_t ads1256_read_differential(ads1256_ain_t pos_ch, ads1256_ain_t neg_ch);

/**
 * @brief Puts the ADC in continuous read mode on a specific channel.
 * After this, call ads1256_read_continuous_data() to get samples.
 * @param channel The analog input channel to read from.
 */
void ads1256_start_continuous_se(ads1256_ain_t channel);

/**
 * @brief Takes the ADC out of continuous read mode.
 */
void ads1256_stop_continuous(void);

/**
 * @brief Reads the next sample in continuous mode.
 * Blocks until the DRDY pin indicates data is ready.
 * @return The 24-bit signed conversion result.
 */
int32_t ads1256_read_continuous_data(void);


#endif // ADS1256_H