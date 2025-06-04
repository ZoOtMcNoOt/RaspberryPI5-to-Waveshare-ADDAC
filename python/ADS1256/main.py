#!/usr/bin/python
# -*- coding:utf-8 -*-

import time
import ADS1256
import config_patched as config

try:
    ADC = ADS1256.ADS1256()
    result = ADC.ADS1256_init()
    
    if result == -1:
        print("ADC initialization failed!")
        exit(1)
    
    print("ADC initialized successfully! Starting measurements...")
    while(1):
        ADC_Value = ADC.ADS1256_GetAll()
        print ("0 ADC = %lf"%(ADC_Value[0]*5.0/0x7fffff))
        print ("1 ADC = %lf"%(ADC_Value[1]*5.0/0x7fffff))
        print ("2 ADC = %lf"%(ADC_Value[2]*5.0/0x7fffff))
        print ("3 ADC = %lf"%(ADC_Value[3]*5.0/0x7fffff))
        print ("4 ADC = %lf"%(ADC_Value[4]*5.0/0x7fffff))
        print ("5 ADC = %lf"%(ADC_Value[5]*5.0/0x7fffff))
        print ("6 ADC = %lf"%(ADC_Value[6]*5.0/0x7fffff))
        print ("7 ADC = %lf"%(ADC_Value[7]*5.0/0x7fffff))
        print ("\33[9A")

        

except KeyboardInterrupt:
    print("\r\nExiting...")
except Exception as e:
    print(f"Error: {e}")
finally:
    try:
        config.destroy()
        print("\r\nProgram end     ")
    except:
        print("\r\nCleanup failed, but program ended")