/*
	Simple compilation test for ADS1256 libgpiod driver
	This file tests that all headers and types are properly defined
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "ads1256_libgpiod.h"

int main() {
    printf("Testing type definitions...\n");
    
    // Test basic types
    uint8_t test_uint8 = 255;
    uint16_t test_uint16 = 65535;
    uint32_t test_uint32 = 4294967295U;
    int32_t test_int32 = -2147483648;
    
    printf("uint8_t: %u\n", test_uint8);
    printf("uint16_t: %u\n", test_uint16);
    printf("uint32_t: %u\n", test_uint32);
    printf("int32_t: %d\n", test_int32);
    
    // Test enum values
    printf("PGA_GAIN1: %d\n", PGA_GAIN1);
    printf("DRATE_30000: 0x%02X\n", DRATE_30000);
    printf("AIN0: %d\n", AIN0);
    
    // Test boolean
    bool test_bool = True;
    printf("bool True: %d\n", test_bool);
    
    printf("All types compiled successfully!\n");
    printf("Note: This test only verifies compilation, not hardware functionality.\n");
    
    return 0;
}