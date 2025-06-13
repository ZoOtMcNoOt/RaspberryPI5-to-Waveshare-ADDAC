/*
	Simple test program for ADS1256 driver with libgpiod
	Tests basic functionality and verifies the conversion from bcm2835
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "ads1256_libgpiod.h"

void test_initialization(void);
void test_chip_id(void);
void test_single_channel_read(void);
void test_multiple_channels(void);
void test_differential_mode(void);
void print_voltage(int32_t raw_value, const char* label);

int main(int argc, char *argv[])
{
    printf("=== ADS1256 libgpiod Driver Test ===\n\n");
    
    // Test 1: Initialization
    printf("1. Testing initialization...\n");
    test_initialization();
    
    // Test 2: Chip ID
    printf("\n2. Testing chip ID read...\n");
    test_chip_id();
    
    // Test 3: Basic configuration
    printf("\n3. Configuring ADS1256...\n");
    setBuffer(True);
    setPGA(PGA_GAIN1);  // ±5V range
    setDataRate(DRATE_100);  // 100 SPS for stable readings
    printf("   Buffer: Enabled\n");
    printf("   PGA: GAIN1 (±5V)\n");
    printf("   Data Rate: 100 SPS\n");
    
    // Test 4: Single channel read
    printf("\n4. Testing single channel read...\n");
    test_single_channel_read();
    
    // Test 5: Multiple channels
    printf("\n5. Testing multiple channel scan...\n");
    test_multiple_channels();
    
    // Test 6: Differential mode
    printf("\n6. Testing differential mode...\n");
    test_differential_mode();
    
    // Cleanup
    printf("\n7. Cleaning up...\n");
    endSPI();
    printf("   GPIO and SPI resources released\n");
    
    printf("\n=== Test Complete ===\n");
    return 0;
}

void test_initialization(void)
{
    if (initializeSPI()) {
        printf("   ✓ SPI and GPIO initialization successful\n");
        printf("   ✓ GPIO lines configured\n");
        printf("   ✓ SPI device opened\n");
    } else {
        printf("   ✗ Initialization failed\n");
        printf("   Check:\n");
        printf("     - SPI is enabled (raspi-config)\n");
        printf("     - Running with sudo\n");
        printf("     - libgpiod is installed\n");
        printf("     - Correct GPIO chip (gpiochip4 for RPi5)\n");
        exit(1);
    }
}

void test_chip_id(void)
{
    // Small delay to ensure chip is ready
    delayus(1000);
    
    uint8_t chip_id = readChipID();
    printf("   Chip ID: 0x%02X", chip_id);
    
    // ADS1256 should return ID of 3
    if (chip_id == 3) {
        printf(" ✓ (Valid ADS1256)\n");
    } else if (chip_id == 0 || chip_id == 0xFF) {
        printf(" ✗ (No response - check connections)\n");
    } else {
        printf(" ? (Unexpected ID - may still work)\n");
    }
}

void test_single_channel_read(void)
{
    printf("   Reading AIN0 (single-ended):\n");
    
    for (int i = 0; i < 3; i++) {
        waitDRDY();
        int32_t value = getValSEChannel(AIN0);
        printf("     Measurement %d: ", i+1);
        print_voltage(value, "");
        
        // Small delay between readings
        delayus(10000);
    }
}

void test_multiple_channels(void)
{
    uint8_t channels[] = {AIN0, AIN1, AIN2, AIN3};
    uint32_t values[4];
    int num_channels = 4;
    
    printf("   Scanning channels 0-3:\n");
    scanSEChannels(channels, num_channels, values);
    
    for (int i = 0; i < num_channels; i++) {
        printf("     AIN%d: ", i);
        print_voltage((int32_t)values[i], "");
    }
}

void test_differential_mode(void)
{
    printf("   Reading AIN0-AIN1 (differential):\n");
    
    waitDRDY();
    int32_t diff_value = getValDIFFChannel(AIN0, AIN1);
    printf("     AIN0-AIN1: ");
    print_voltage(diff_value, "");
    
    printf("   Reading AIN0-AINCOM (single-ended via differential):\n");
    waitDRDY();
    int32_t se_value = getValDIFFChannel(AIN0, AINCOM);
    printf("     AIN0-AINCOM: ");
    print_voltage(se_value, "");
}

void print_voltage(int32_t raw_value, const char* label)
{
    // Convert to voltage (assumes GAIN1 and 5V reference)
    double voltage = (double)raw_value / 1670000.0;
    printf("%s%.6f V (raw: %d)\n", label, voltage, raw_value);
}