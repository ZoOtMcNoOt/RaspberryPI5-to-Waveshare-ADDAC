#!/usr/bin/python3
# -*- coding:utf-8 -*-
"""
AD-DA converter main application.
Reads ADC values and outputs corresponding DAC voltages.
"""

import sys
import ADS1256
import DAC8532
import config_patched as config

ADC = ADS1256.ADS1256()
DAC = DAC8532.DAC8532()
ADC.ADS1256_init()

try:
    while True:

        ADC_Value = ADC.ADS1256_GetAll()
        print(f"0 ADC = {ADC_Value[0]*5.0/0x7fffff}")
        print(f"1 ADC = {ADC_Value[1]*5.0/0x7fffff}")
        print(f"2 ADC = {ADC_Value[2]*5.0/0x7fffff}")
        print(f"3 ADC = {ADC_Value[3]*5.0/0x7fffff}")
        print(f"4 ADC = {ADC_Value[4]*5.0/0x7fffff}")
        print(f"5 ADC = {ADC_Value[5]*5.0/0x7fffff}")
        print(f"6 ADC = {ADC_Value[6]*5.0/0x7fffff}")
        print(f"7 ADC = {ADC_Value[7]*5.0/0x7fffff}")

        dac_voltage = (ADC_Value[0]>>7)*5.0/0xffff  # pylint: disable=invalid-name
        print(f"DAC : {dac_voltage}")
        print("\33[10A")
        DAC.DAC8532_Out_Voltage(DAC8532.channel_A, dac_voltage)
        DAC.DAC8532_Out_Voltage(DAC8532.channel_B, 5.0 - dac_voltage)

except KeyboardInterrupt:
    DAC.DAC8532_Out_Voltage(DAC8532.channel_A, 0)
    DAC.DAC8532_Out_Voltage(DAC8532.channel_B, 0)
    config.destroy()
    print("\r\nProgram end     ")
    sys.exit()
