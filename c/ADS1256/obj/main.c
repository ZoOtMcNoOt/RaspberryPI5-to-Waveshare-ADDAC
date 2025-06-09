#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "ADS1256.h"
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
    UDOUBLE ADC[8],i;
    struct timespec start_time, current_time;
    unsigned long sample_count = 0;
    double elapsed_time, sample_rate;
    
    printf("demo\r\n");
    DEV_ModuleInit();

    // Exception handling:ctrl + c
    signal(SIGINT, Handler);

    if(ADS1256_init() == 1){
        printf("\r\nEND                  \r\n");
        DEV_ModuleExit();
        exit(0);
    }

    // Get initial time
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while(1){
        ADS1256_GetAll_Fast(ADC); // Use the optimized version
        sample_count++;
        
        // Calculate elapsed time and sample rate
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) + 
                      (current_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
        
        if(elapsed_time > 0) {
            sample_rate = sample_count / elapsed_time;
        } else {
            sample_rate = 0;
        }
        
        for(i=0;i<8;i++){
            printf("%d %f\r\n",i,ADC[i]*5.0/0x7fffff);
        }
        printf("Samples/sec: %.2f (Total: %lu)\r\n", sample_rate, sample_count);
        printf("\33[9A");//Move the cursor up 9 lines (8 channels + 1 sample rate line)
    }
    return 0;
}