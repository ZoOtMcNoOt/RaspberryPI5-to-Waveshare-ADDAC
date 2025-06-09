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

    gpiod_line_request_output(rst_line, "AD-DA", 1);
    gpiod_line_request_output(cs_line, "AD-DA", 1);
    gpiod_line_request_output(cs1_line, "AD-DA", 1);
    gpiod_line_request_input(drdy_line, "AD-DA");
}

/*
    * function: Module initialization, opens the SPI device, configures it, and initializes GPIO lines
    * parameter: None
    * Info:
    * This function initializes the SPI device and GPIO lines for communication.
*/

int DEV_ModuleInit(void) {
    // Open the SPI device
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) {
        perror("Failed to open SPI device");
        return 1;
    }

    // Set SPI mode, bits per word, and speed
    uint8_t mode = SPI_MODE_1;
    uint8_t bits = 8;
    uint32_t speed = 10000000; // 10 MHz (increased from 1 MHz)

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("Failed to configure SPI");      // If any of the ioctl calls fail, print an error message and exit
        close(spi_fd);
        return 1;
    }

    //Open the GPIO chip
    // Note: Using gpiochip4 which contains the main GPIO pins on Raspberry Pi 5
    gpio_chip = gpiod_chip_open_by_name("gpiochip4");
    if (!gpio_chip) {
        perror("Failed to open GPIO chip");
        close(spi_fd);
        return 1;
    }

    //Configure GPIO
    DEV_GPIOConfig();
    Debug("DEV_ModuleInit: SPI and GPIO initialized successfully.\n");
    return 0;
}

/*
    * function: Write a byte to the SPI bus
    * parameter: value - the byte value to write
    * Info:
    * This function sends a single byte over the SPI bus and receives the response.
*/

void SPI_WriteByte(uint8_t value) {
    uint8_t rx;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)&value,
        .rx_buf = (unsigned long)&rx,
        .len = 1,
        .speed_hz = 10000000, // 10 MHz
        .bits_per_word = 8,
    };
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("Failed to write SPI byte");
    }
}

UBYTE SPI_ReadByte(void) {
    uint8_t tx =0xFF;
    uint8_t rx = 0;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)&tx,
        .rx_buf = (unsigned long)&rx,
        .len = 1,
        .speed_hz = 10000000, // 10 MHz
        .bits_per_word = 8,
    };
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("Failed to read SPI byte");
        return 0;
    }
    return rx;
}

void DEV_GPIO_Write(int pin, int value) {
    struct gpiod_line *line = NULL;

    switch (pin) {
        case DEV_RST_PIN:
            line = rst_line;
            break;
        case DEV_CS_PIN:
            line = cs_line;
            break;
        case DEV_CS1_PIN:
            line = cs1_line;
            break;
        default:
            fprintf(stderr, "Invalid GPIO pin: %d\n", pin);
            return;
    }

    if (line) {
        gpiod_line_set_value(line, value);
    } else {
        fprintf(stderr, "Failed to write to GPIO pin: %d\n", pin);
    }
}

int DEV_GPIO_Read (int pin){
    if (pin == DEV_DRDY_PIN && drdy_line) {
        return gpiod_line_get_value(drdy_line);
    }
    return 0;
}

void DEV_ModuleExit(void) {
    if (rst_line) gpiod_line_release(rst_line);
    if (cs_line) gpiod_line_release(cs_line);
    if (cs1_line) gpiod_line_release(cs1_line);
    if (drdy_line) gpiod_line_release(drdy_line);
    if (gpio_chip) gpiod_chip_close(gpio_chip);
    if (spi_fd >= 0) close(spi_fd);
    Debug("DEV_ModuleExit: SPI and GPIO resources released.\n");
}