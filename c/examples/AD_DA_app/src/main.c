#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "stdio.h"

#include "../../lib/ADS1256/ADS1256.h"
#include "../../lib/DAC8532/DAC8532.h"
#include "../../common/Debug.h" // Assuming Debug.h is used

void Handler(int signo)
{
    //System Exit
    printf("\r\nEND                  \r\n");
    DEV_ModuleExit();

    exit(0);
}

int main(void)
{
    UDOUBLE ADC[8], i;
    float x;

    printf("demo\r\n");
    DEV_ModuleInit();

    // Exception handling: ctrl + c
    signal(SIGINT, Handler);

    if (ADS1256_init() == 1) {
        printf("\r\nEND                  \r\n");
        DEV_ModuleExit();
        exit(0);
    }

    while (1) {
        ADS1256_GetAll(ADC);
        for (i = 0; i < 8; i++) {
            printf("%d %f\r\n", i, ADC[i] * 5.0 / 0x7fffff);
        }

        x = (ADC[0] >> 7) * 5.0 / 0xffff;
        printf(" %f \r\n", x);
        printf("\33[9A"); // Move the cursor up 8 lines
        
        DAC8532_Out_Voltage(channel_B, (x));
        DAC8532_Out_Voltage(channel_A, (5.0 - x));
    }
    return 0;
}