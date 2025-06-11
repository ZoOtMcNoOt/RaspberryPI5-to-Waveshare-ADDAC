#include "ADS1256.h"
#include <sys/time.h>


UBYTE ScanMode = 0;

/*
    * function:   Module reset
    * parameter:
    * Info:
    * This function resets the ADS1256 module by toggling the reset pin.
*/

static void ADS1256_reset(void)
{
    DEV_Digital_Write(DEV_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(DEV_RST_PIN, 0);
    DEV_Delay_ms(200);
    DEV_Digital_Write(DEV_RST_PIN, 1);
}

/*
    * function:   send command
    * parameter: 
    *         Cmd: command
    * Info:
    * This function sends a command to the ADS1256 module by writing to the chip select pin and using SPI.
*/

static void ADS1256_WriteCmd(UBYTE Cmd)
{
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(Cmd);
    DEV_Digital_Write(DEV_CS_PIN, 1);
}

/*
    * function:   Write a data to the destination register
    * parameter: 
    *         Reg : Target register
    *         data: Written data
    * Info:
    * This function writes data to a specified register in the ADS1256 module.
*/
static void ADS1256_WriteReg(UBYTE Reg, UBYTE data)
{
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_WREG | Reg);
    DEV_SPI_WriteByte(0x00);
    DEV_SPI_WriteByte(data);
    DEV_Digital_Write(DEV_CS_PIN, 1);
}

/*
    * function:   Read a data from the destination register
    * parameter: 
    *         Reg : Target register
    * Info:
    * This function reads data from a specified register in the ADS1256 module.
    * Returns the read data.
*/

static UBYTE ADS1256_Read_data(UBYTE Reg)
{
    UBYTE temp = 0;
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_RREG | Reg);
    DEV_SPI_WriteByte(0x00);
    DEV_Delay_ms(1); // Allow time for the read operation to complete
    temp = DEV_SPI_ReadByte();
    DEV_Digital_Write(DEV_CS_PIN, 1);
    return temp;
}

/*
    * function: Wait for the Data Ready (DRDY) signal
    * parameter: None
    * Info:
    * This function waits for the DRDY pin to go low, indicating that data is ready to be read.
    * If the DRDY pin does not go low within a specified timeout period, it prints a timeout message.
*/

static void ADS1256_WaitDRDY(void)
{
    UDOUBLE i = 0;
    for(i = 0; i< 4000000; i++) {
        if(DEV_Digital_Read(DEV_DRDY_PIN) == 0) {
            break;
        }
    }
    if (i >= 4000000) {
        printf("ADS1256 Time Out! ...\n");
    }
}

/*
    * function: Read the chip ID
    * parameter: None
    * Info:
    * This function reads the chip ID from the ADS1256 module.
    * Returns the chip ID.
*/
UBYTE ADS1256_ReadChipID(void)
{
    UBYTE id;
    ADS1256_WaitDRDY(); // Wait for DRDY to go low before reading
    id = ADS1256_Read_data(REG_STATUS); 
    return id>>4; // Return the chip ID by shifting right 4 bits
}

void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate)
{
    ADS1256_WaitDRDY(); // Wait for DRDY to go low before configuring
    UBYTE buf[4] = {0, 0, 0, 0};

    buf[0] = (0<<3) | (1<<2) | (1<<1); // Set the ADC to normal mode, gain 1, and continuous conversion
    buf[1] = 0x08 ; // Set the ADC to 8 channels, AIN0 as the input channel
    buf[2] = (0<<5) | (0<<3) | (gain<<0); // Set the gain in the ADCON register
    buf[3] = ADS1256_DRATE_E[drate]; // Set the data rate in the DRATE register
    
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_WREG | 0);
    DEV_SPI_WriteByte(0x03);

    DEV_SPI_WriteByte(buf[0]);
    DEV_SPI_WriteByte(buf[1]);
    DEV_SPI_WriteByte(buf[2]);
    DEV_SPI_WriteByte(buf[3]);
    DEV_Digital_Write(DEV_CS_PIN, 1);
    DEV_Delay_ms(1); // Allow time for the configuration to take effect
}

static void ADS1256_SetChannel(UBYTE Channel) {
    if (Channel > 7) {
        return;
    }
    ADS1256_WriteReg(REG_MUX, (Channel << 4) | (1<<3)); // Set the channel in the MUX register
}

static void ADS1256_SetDiffChannal(UBYTE Channal) {
    if (Channal == 0){
        ADS1256_WriteReg(REG_MUX, (0 << 4) | 1); // Set differential channel AIN0-AIN1
    } else if (Channal == 1) {
        ADS1256_WriteReg(REG_MUX, (2 << 4) | 3); // Set differential channel AIN2-AIN3
    } else if (Channal == 2) {
        ADS1256_WriteReg(REG_MUX, (4 << 4) | 5); // Set differential channel AIN4-AIN5
    } else if (Channal == 3) {
        ADS1256_WriteReg(REG_MUX, (6 << 4) | 7); // Set differential channel AIN6-AIN7
    }
}

void ADS1256_SetMode(UBYTE Mode) {
    if (Mode == 0) {
        ScanMode = 0; // Single-ended input mode
    } else if (Mode == 1) {
        ScanMode = 1; // Differential input mode
    }
}

UBYTE ADS1256_init(void) {
    ADS1256_reset();
    if(ADS1256_ReadChipID() == 3) {
        printf("ID Read success \r\n");
    } else {
        printf("ID Read failed \r\n");
        return 1; // Initialization failed
    }
    ADS1256_ConfigADC(ADS1256_GAIN_1, ADS1256_30000SPS); // Configure the ADC with gain 1 and data rate 30k SPS
    return 0;
}

static UDOUBLE ADS1256_read_ADC_Data(void) {
    UDOUBLE read =0;
    UBYTE buf[3] = {0, 0, 0};

    ADS1256_WaitDRDY(); // Wait for DRDY to go low before reading
    DEV_Delay_ms(1); // Allow time for the ADC to prepare data
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_RDATA); // Send the read data command
    buf[0] = DEV_SPI_ReadByte(); // Read the first byte
    buf[1] = DEV_SPI_ReadByte(); // Read the second byte
    buf[2] = DEV_SPI_ReadByte(); // Read the third byte
    DEV_Digital_Write(DEV_CS_PIN, 1); // Deselect the chip
    read = ((UDOUBLE)buf[0] << 16) & 0x00FF0000; // Combine the bytes into a single value
    read |= ((UDOUBLE)buf[1] << 8);  // Combine the second byte
    read |= buf[2]; // Combine the third byte
    if (read & 0x800000) {
        read |= 0xFF000000; // Sign extend if the most significant bit is set
    }
    return read; // Return the combined value
}

UDOUBLE ADS1256_GetChannalValue(UBYTE Channel) {
    UDOUBLE Value = 0;
    while(DEV_Digital_Read(DEV_DRDY_PIN) == 1); // Wait for DRDY to go low
    if(ScanMode == 0) { // Single-ended input mode (8 channels  )
        if(Channel >= 8) {
            return 0; // Invalid channel
        }
        ADS1256_SetChannel(Channel); // Set the channel
        ADS1256_WriteCmd(CMD_SYNC); // Synchronize the ADC
        ADS1256_WriteCmd(CMD_WAKEUP); // Wake up the ADC
        Value = ADS1256_read_ADC_Data(); // Read the ADC data
    } else { // Differential input mode (4 channels)
        if(Channel >= 4) {
            return 0; // Invalid channel
        }
        ADS1256_SetDiffChannal(Channel); // Set the differential channel
        ADS1256_WriteCmd(CMD_SYNC); // Synchronize the ADC
        ADS1256_WriteCmd(CMD_WAKEUP); // Wake up the ADC
        Value = ADS1256_read_ADC_Data(); // Read the ADC data
    }
    return Value; // Return the read value
}

void ADS1256_GetAll(UDOUBLE *ADC_Value) {
    UBYTE i;

    for(i=0; i<8; i++) {
        ADC_Value[i] = ADS1256_GetChannalValue(i);
    }
}  


// Performance tracking structure
typedef struct {
    double theoretical_max_per_channel;  // SPS per channel
    double theoretical_total;            // Total SPS across all channels
    double actual_per_channel;           // Measured SPS per channel
    double actual_total;                 // Measured total SPS
    double efficiency_percent;           // Actual vs theoretical efficiency
    unsigned long total_scans;           // Total scan cycles completed
    struct timeval start_time;           // Start time for measurements
} performance_metrics_t;

static performance_metrics_t perf_metrics = {0};

// Fixed sign extension function
static UDOUBLE ADS1256_fix_sign_extension(UDOUBLE raw_value) {
    // Fix the sign extension bug in your original code
    if (raw_value & 0x800000) {
        raw_value |= 0xFF000000;  // Proper sign extension (was &= 0xFF000000)
    }
    return raw_value;
}

// Enhanced read with proper settling time (5 DRDY periods)
static UDOUBLE ADS1256_read_ADC_Data_settled(void) {
    UDOUBLE read = 0;
    UBYTE buf[3] = {0, 0, 0};
    
    // Wait for 5 DRDY periods to ensure proper settling
    for(int settle_count = 0; settle_count < 5; settle_count++) {
        // Wait for DRDY to go low
        while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
        
        // Only read data on the final settled conversion
        if(settle_count == 4) {
            DEV_Digital_Write(DEV_CS_PIN, 0);
            DEV_SPI_WriteByte(CMD_RDATA);
            buf[0] = DEV_SPI_ReadByte();
            buf[1] = DEV_SPI_ReadByte(); 
            buf[2] = DEV_SPI_ReadByte();
            DEV_Digital_Write(DEV_CS_PIN, 1);
        }
        
        // Wait for DRDY to go high before next cycle (except on last iteration)
        if(settle_count < 4) {
            while(DEV_Digital_Read(DEV_DRDY_PIN) == 0);
        }
    }
    
    // Combine bytes with proper sign extension
    read = ((UDOUBLE)buf[0] << 16) & 0x00FF0000;
    read |= ((UDOUBLE)buf[1] << 8);
    read |= buf[2];
    
    return ADS1256_fix_sign_extension(read);
}

// Optimized 4-channel acquisition with proper settling
void ADS1256_Get4Channels_Optimized(UDOUBLE *ADC_Value, UBYTE *channels) {
    struct timeval scan_start;
    gettimeofday(&scan_start, NULL);
    
    for(UBYTE i = 0; i < 4; i++) {
        UBYTE channel = channels[i];
        
        // Validate channel number
        if(channel > 7) {
            ADC_Value[i] = 0;
            continue;
        }
        
        // Set the channel
        ADS1256_WriteReg(REG_MUX, (channel << 4) | (1<<3));
        
        // Sync and wake up the ADC
        ADS1256_WriteCmd(CMD_SYNC);
        ADS1256_WriteCmd(CMD_WAKEUP);
        
        // Read with proper settling time
        ADC_Value[i] = ADS1256_read_ADC_Data_settled();
    }
    
    // Update performance metrics
    perf_metrics.total_scans++;
    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    double elapsed = (current_time.tv_sec - perf_metrics.start_time.tv_sec) + 
                    (current_time.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;
    
    if(elapsed > 0) {
        perf_metrics.actual_per_channel = perf_metrics.total_scans / elapsed;
        perf_metrics.actual_total = perf_metrics.actual_per_channel * 4;
        perf_metrics.efficiency_percent = (perf_metrics.actual_per_channel / perf_metrics.theoretical_max_per_channel) * 100.0;
    }
}

// Alternative: Ultra-fast 4-channel with minimal overhead
void ADS1256_Get4Channels_Fast(UDOUBLE *ADC_Value, UBYTE *channels) {
    for(UBYTE i = 0; i < 4; i++) {
        UBYTE channel = channels[i];
        
        if(channel > 7) {
            ADC_Value[i] = 0;
            continue;
        }
        
        // Combined channel set + sync + wakeup in single SPI transaction
        DEV_Digital_Write(DEV_CS_PIN, 0);
        DEV_SPI_WriteByte(CMD_WREG | REG_MUX);
        DEV_SPI_WriteByte(0x00);
        DEV_SPI_WriteByte((channel << 4) | (1<<3));
        DEV_SPI_WriteByte(CMD_SYNC);
        DEV_SPI_WriteByte(CMD_WAKEUP);
        DEV_Digital_Write(DEV_CS_PIN, 1);
        
        // Quick settling - wait for fewer DRDY cycles (faster but less accurate)
        for(int settle = 0; settle < 3; settle++) {
            while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
            if(settle < 2) {
                while(DEV_Digital_Read(DEV_DRDY_PIN) == 0);
            }
        }
        
        // Read data
        UBYTE buf[3];
        DEV_Digital_Write(DEV_CS_PIN, 0);
        DEV_SPI_WriteByte(CMD_RDATA);
        buf[0] = DEV_SPI_ReadByte();
        buf[1] = DEV_SPI_ReadByte();
        buf[2] = DEV_SPI_ReadByte();
        DEV_Digital_Write(DEV_CS_PIN, 1);
        
        UDOUBLE read = ((UDOUBLE)buf[0] << 16) & 0x00FF0000;
        read |= ((UDOUBLE)buf[1] << 8);
        read |= buf[2];
        
        ADC_Value[i] = ADS1256_fix_sign_extension(read);
    }
    
    perf_metrics.total_scans++;
}

// Initialize performance monitoring for 4 channels
void ADS1256_InitPerformanceMonitoring_4Ch(void) {
    // Theoretical limits for 4 channels from datasheet Table 14
    // At 30kSPS with 4 channels: ~8748 SPS per channel, ~34992 total
    perf_metrics.theoretical_max_per_channel = 8748.0;  // From datasheet
    perf_metrics.theoretical_total = perf_metrics.theoretical_max_per_channel * 4;
    perf_metrics.actual_per_channel = 0;
    perf_metrics.actual_total = 0;
    perf_metrics.efficiency_percent = 0;
    perf_metrics.total_scans = 0;
    
    gettimeofday(&perf_metrics.start_time, NULL);
    
    printf("=== ADS1256 4-Channel Performance Monitor Initialized ===\n");
    printf("Theoretical Max: %.0f SPS per channel (%.0f total)\n", 
           perf_metrics.theoretical_max_per_channel, perf_metrics.theoretical_total);
    printf("Based on ADS1256 datasheet Table 14 with proper settling time\n\n");
}

// Get current performance metrics
performance_metrics_t* ADS1256_GetPerformanceMetrics(void) {
    return &perf_metrics;
}

// Print detailed performance report
void ADS1256_PrintPerformanceReport(void) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    double elapsed = (current_time.tv_sec - perf_metrics.start_time.tv_sec) + 
                    (current_time.tv_usec - perf_metrics.start_time.tv_usec) / 1000000.0;
    
    printf("\n=== ADS1256 4-Channel Performance Report ===\n");
    printf("Runtime: %.2f seconds\n", elapsed);
    printf("Total Scan Cycles: %lu\n", perf_metrics.total_scans);
    printf("Theoretical Max: %.0f SPS/ch (%.0f total)\n", 
           perf_metrics.theoretical_max_per_channel, perf_metrics.theoretical_total);
    printf("Actual Rate: %.1f SPS/ch (%.1f total)\n", 
           perf_metrics.actual_per_channel, perf_metrics.actual_total);
    printf("Efficiency: %.1f%%\n", perf_metrics.efficiency_percent);
    
    if(perf_metrics.efficiency_percent > 90) {
        printf("Status: EXCELLENT - Near theoretical limits\n");
    } else if(perf_metrics.efficiency_percent > 75) {
        printf("Status: GOOD - Acceptable performance\n");
    } else if(perf_metrics.efficiency_percent > 50) {
        printf("Status: FAIR - Consider optimization\n");
    } else {
        printf("Status: POOR - Significant optimization needed\n");
    }
    printf("=============================================\n\n");
}

// Real-time efficiency calculation
double ADS1256_GetCurrentEfficiency(void) {
    return perf_metrics.efficiency_percent;
}

// Check if performance is meeting expectations
int ADS1256_IsPerformanceGood(void) {
    return (perf_metrics.efficiency_percent > 75.0 && perf_metrics.total_scans > 10);
}

// Convert raw ADC value to voltage (for 5V reference, gain 1)
float ADS1256_RawToVoltage(UDOUBLE raw_value) {
    // Handle sign extension for negative values
    int32_t signed_value = (int32_t)raw_value;
    if(raw_value & 0x800000) {
        signed_value = (int32_t)(raw_value | 0xFF000000);
    }
    
    // Convert to voltage: ADC range is ±2.5V for 5V reference with gain 1
    // Full scale is 0x7FFFFF = 8388607
    return (float)signed_value * 5.0 / 0x7FFFFF;
}
