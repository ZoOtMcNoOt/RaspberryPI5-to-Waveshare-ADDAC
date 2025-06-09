#include "DEV_Config.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <time.h>
#include <string.h>


// globl device handles
static int spi_fd = -1;
static struct gpiod_chip *gpio_chip = NULL;
static struct gpiod_line *rst_line = NULL;
static struct gpiod_line *cs_line = NULL;
static struct gpiod_line *cs1_line = NULL;
static struct gpiod_line *drdy_line = NULL;

void DEV_Delay_ms_func(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/*
    * function: Initialize the SPI device and GPIO lines
    * parameter: None
    * Info:
    * This function initializes the SPI device and GPIO lines for communication with the ADS1256 module.
*/

static void DEV_GPIOConfig(void) {
    rst_line = gpiod_chip_get_line(gpio_chip, DEV_RST_PIN);
    cs_line = gpiod_chip_get_line(gpio_chip, DEV_CS_PIN);
    cs1_line = gpiod_chip_get_line(gpio_chip, DEV_CS1_PIN);
    drdy_line = gpiod_chip_get_line(gpio_chip, DEV_DRDY_PIN);

    if (!rst_line || !cs_line || !cs1_line || !drdy_line) {
        perror("Failed to get GPIO lines");
        return;
    }

    gpiod_line_request_output(rst_line, "ADS1256", 1);
    gpiod_line_request_output(cs_line, "ADS1256", 1);
    gpiod_line_request_output(cs1_line, "ADS1256", 1);
    gpiod_line_request_input(drdy_line, "ADS1256");

}