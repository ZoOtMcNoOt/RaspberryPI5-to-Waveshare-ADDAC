#!/usr/bin/python
# -*- coding:utf-8 -*-
"""
ADS1256 ADC measurement application with update rate monitoring.
Reads all 8 ADC channels and displays the sampling rate.
"""

import time
import ADS1256
import config_patched as config

try:
    ADC = ADS1256.ADS1256()
    result = ADC.ADS1256_init()  # pylint: disable=invalid-name
    
    if result == -1:
        print("ADC initialization failed!")
        exit(1)
    
    print("ADC initialized successfully! Starting measurements...")
    
    # Variables for measuring update speed
    start_time = time.time()
    sample_count = 0  # pylint: disable=invalid-name
    
    while True:
        current_time = time.time()
        sample_count += 1
        
        ADC_Value = ADC.ADS1256_GetAll()
        
        # Calculate update rate
        elapsed_time = current_time - start_time
        update_rate = sample_count / elapsed_time if elapsed_time > 0 else 0
        
        # Clear screen and print all data
        print("\033[2J\033[H", end="")  # Clear screen and move cursor to top
        print("=== ADS1256 ADC Readings ===")
        print(f"0 ADC = {ADC_Value[0]*5.0/0x7fffff:.6f} V")
        print(f"1 ADC = {ADC_Value[1]*5.0/0x7fffff:.6f} V")
        print(f"2 ADC = {ADC_Value[2]*5.0/0x7fffff:.6f} V")
        print(f"3 ADC = {ADC_Value[3]*5.0/0x7fffff:.6f} V")
        print(f"4 ADC = {ADC_Value[4]*5.0/0x7fffff:.6f} V")
        print(f"5 ADC = {ADC_Value[5]*5.0/0x7fffff:.6f} V")
        print(f"6 ADC = {ADC_Value[6]*5.0/0x7fffff:.6f} V")
        print(f"7 ADC = {ADC_Value[7]*5.0/0x7fffff:.6f} V")
        print(f"Update Rate: {update_rate:.2f} samples/sec")
        print(f"Total Samples: {sample_count}")
        print("Press Ctrl+C to exit...")

except KeyboardInterrupt:
    print("\r\nExiting...")
except (IOError, OSError, RuntimeError) as e:
    print(f"Error: {e}")
finally:
    try:
        config.destroy()
        print("\r\nProgram end     ")
    except (IOError, OSError, AttributeError):
        print("\r\nCleanup failed, but program ended")