#!/usr/bin/python
# -*- coding:utf-8 -*-


import time
import DAC8532
import config_patched as config


try:
    print("Program start\r\n")
    
    DAC = DAC8532.DAC8532()
    DAC.DAC8532_Out_Voltage(DAC8532.channel_A, 0)
    DAC.DAC8532_Out_Voltage(DAC8532.channel_B, 0)
    

    while(1):
        for i in range(0,50,1):
            DAC.DAC8532_Out_Voltage(DAC8532.channel_A, 5.0 * i / 50)
            DAC.DAC8532_Out_Voltage(DAC8532.channel_B, 5.0 - 5.0 * i / 50)
            time.sleep(0.2)
            
        for i in range(0,50,1):
            DAC.DAC8532_Out_Voltage(DAC8532.channel_B, 5.0 * i / 50)
            DAC.DAC8532_Out_Voltage(DAC8532.channel_A, 5.0 - 5.0 * i / 50)
            time.sleep(0.2)
    
except :
    GPIO.cleanup()
    print ("\r\nProgram end     ")
    exit()
