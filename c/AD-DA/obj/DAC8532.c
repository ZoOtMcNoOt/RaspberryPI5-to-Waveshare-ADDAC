#include "DAC8532.h"

void Write_DAC8532(UBYTE Channel, UWORD Data)
{
    DEV_Digital_Write(DEV_CS1_PIN, 1);
    DEV_Digital_Write(DEV_CS1_PIN, 0);
    DEV_SPI_WriteByte(Channel);
    DEV_SPI_WriteByte((Data >> 8));
    DEV_SPI_WriteByte((Data & 0xff));  
    DEV_Digital_Write(DEV_CS1_PIN, 1);
}

void DAC8532_Out_Voltage(UBYTE Channel, float Voltage)
{
    UWORD temp = 0;
    if ((Voltage <= DAC_VREF) && (Voltage >= 0)) {
        temp = (UWORD)(Voltage * DAC_Value_MAX / DAC_VREF);
        Write_DAC8532(Channel, temp);
    }
}