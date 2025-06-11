#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "../../lib/DAC8532/DAC8532.h"
#include "../../common/Debug.h" // Assuming Debug.h is used, DEV_Config.h is included by DAC8532.h
#include "stdio.h"
#include <string.h>

void Handler(int signo)
{
    //System Exit
    printf("\r\nEND                  \r\n");
    DEV_ModuleExit();
    exit(0);
}

int main(void)
{
    UDOUBLE i;
    DEV_ModuleInit();

    // Exception handling:ctrl + c
    signal(SIGINT, Handler);
    
    printf("Program start\r\n");
    
    DAC8532_Out_Voltage(channel_A, 0);
    while(1){
        for(i=0;i<50;i++){
            DAC8532_Out_Voltage(channel_A, DAC_VREF * i / 50);
            DAC8532_Out_Voltage(channel_B, DAC_VREF - DAC_VREF * i / 50);
            DEV_Delay_ms(100);
        }
        for(i=0;i<50;i++){
            DAC8532_Out_Voltage(channel_B, DAC_VREF * i / 50);
            DAC8532_Out_Voltage(channel_A, DAC_VREF - DAC_VREF * i / 50);
            DEV_Delay_ms(100);
        }
    }
    return 0;
}