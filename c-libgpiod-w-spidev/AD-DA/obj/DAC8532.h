#ifndef _DAC8532_H_
#define _DAC8532_H_

#include "DEV_Config.h"

#define channel_A   0x30
#define channel_B   0x34
#define DAC_Value_MAX  65535
#define DAC_VREF  3.3

void DAC_Out_Voltage(UBYTE Channel, float Voltage);
void Write_DAC8532(UBYTE Channel, UWORD Data);

#endif