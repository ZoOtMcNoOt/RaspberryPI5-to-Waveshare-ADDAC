#include <gpiod.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#define CHIP_PATH   "/dev/gpiochip4"   /* Pi‑5 expansion header lives here 
#define LED_LINE    18                 /* BCM numbering (GPIO18 / pin 12) */
#define PERIOD_US   500000             

static volatile sig_atomic_t keep_running = 1;
static void sigint_handler(int) { keep_running = 0; }

int main(void)
{
    struct gpiod_chip *chip   = gpiod_chip_open(CHIP_PATH);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    struct gpiod_line *led = gpiod_chip_get_line(chip, LED_LINE);
    if (!led) { perror("gpiod_chip_get_line"); return 1; }

    if (gpiod_line_request_output(led, "blinky", 0) < 0) {
        perror("gpiod_line_request_output"); return 1;
    }

    signal(SIGINT, sigint_handler);          

    int level = 0;
    while (keep_running) {
        level ^= 1;                         
        gpiod_line_set_value(led, level);
        usleep(PERIOD_US);
    }

    gpiod_line_set_value(led, 0);            
    gpiod_line_release(led);
    gpiod_chip_close(chip);
    return 0;
}