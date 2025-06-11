/**
 * @file DEV_Config.c
 * @brief Device configuration and interface functions for SPI and GPIO.
 *
 * This file implements the low-level hardware interface functions required
 * for communication with peripheral devices using SPI and GPIO on a
 * Raspberry Pi (specifically targeting Raspberry Pi 5 with gpiod).
 * It handles initialization, configuration, data transfer, and cleanup
 * for these interfaces.
 */
#include "DEV_Config.h"
#include <fcntl.h>      // For open()
#include <unistd.h>     // For close(), write(), read()
#include <linux/spi/spidev.h> // For SPI_IOC_MESSAGE, struct spi_ioc_transfer
#include <sys/ioctl.h>  // For ioctl()
#include <time.h>       // For nanosleep()
#include <string.h>     // For memset (though not explicitly used here, good for completeness)

// Global device handles
static int spi_fd = -1;                     // File descriptor for the SPI device
static struct gpiod_chip *gpio_chip = NULL; // GPIO chip handle
static struct gpiod_line *rst_line = NULL;  // GPIO line for Reset pin
static struct gpiod_line *cs_line = NULL;   // GPIO line for ADC Chip Select pin
static struct gpiod_line *cs1_line = NULL;  // GPIO line for DAC Chip Select pin
static struct gpiod_line *drdy_line = NULL; // GPIO line for Data Ready pin

/**
 * @brief Delays execution for a specified number of milliseconds.
 * @param ms The delay time in milliseconds.
 */
void DEV_Delay_ms_func(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L; // Ensure nsec is long
    nanosleep(&ts, NULL);
}

/**
 * @brief Configures the GPIO lines used for device communication.
 *
 * This function requests and configures the necessary GPIO lines (RST, CS, CS1, DRDY)
 * for output or input as required by the connected AD/DA module.
 * It assumes `gpio_chip` has already been successfully opened.
 */
static void DEV_GPIOConfig(void) {
    rst_line = gpiod_chip_get_line(gpio_chip, DEV_RST_PIN);
    cs_line = gpiod_chip_get_line(gpio_chip, DEV_CS_PIN);
    cs1_line = gpiod_chip_get_line(gpio_chip, DEV_CS1_PIN);
    drdy_line = gpiod_chip_get_line(gpio_chip, DEV_DRDY_PIN);

    if (!rst_line || !cs_line || !cs1_line || !drdy_line) {
        perror("DEV_GPIOConfig: Failed to get GPIO lines");
        // Consider more robust error handling, e.g., returning an error code
        return;
    }

    // Request lines with consumer name "AD-DA" and default output value 1 (high)
    if (gpiod_line_request_output(rst_line, "AD-DA", 1) < 0 ||
        gpiod_line_request_output(cs_line, "AD-DA", 1) < 0 ||
        gpiod_line_request_output(cs1_line, "AD-DA", 1) < 0 ||
        gpiod_line_request_input(drdy_line, "AD-DA") < 0) {
        perror("DEV_GPIOConfig: Failed to request GPIO lines");
        // Release any lines that were successfully requested before failing
        if(rst_line && gpiod_line_is_requested(rst_line)) gpiod_line_release(rst_line);
        if(cs_line && gpiod_line_is_requested(cs_line)) gpiod_line_release(cs_line);
        if(cs1_line && gpiod_line_is_requested(cs1_line)) gpiod_line_release(cs1_line);
        if(drdy_line && gpiod_line_is_requested(drdy_line)) gpiod_line_release(drdy_line);
        return;
    }
}

/**
 * @brief Initializes the hardware module.
 *
 * This function performs the following steps:
 * 1. Opens the SPI device (e.g., "/dev/spidev0.0").
 * 2. Configures SPI parameters: mode, bits per word, and max speed.
 * 3. Opens the GPIO chip (e.g., "gpiochip4" for Raspberry Pi 5).
 * 4. Calls DEV_GPIOConfig() to configure individual GPIO lines.
 *
 * @return 0 on success, 1 on failure.
 */
int DEV_ModuleInit(void) {
    // Open the SPI device
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        perror("DEV_ModuleInit: Failed to open SPI device " SPI_DEVICE);
        return 1;
    }

    // Set SPI mode, bits per word, and speed
    uint8_t mode = SPI_MODE_1; // CPOL=0, CPHA=1
    uint8_t bits = 8;
    uint32_t speed = SPI_SPEED_HZ;

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("DEV_ModuleInit: Failed to set SPI_IOC_WR_MODE");
        close(spi_fd);
        spi_fd = -1;
        return 1;
    }
    if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("DEV_ModuleInit: Failed to set SPI_IOC_WR_BITS_PER_WORD");
        close(spi_fd);
        spi_fd = -1;
        return 1;
    }
    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("DEV_ModuleInit: Failed to set SPI_IOC_WR_MAX_SPEED_HZ");
        close(spi_fd);
        spi_fd = -1;
        return 1;
    }

    // Open the GPIO chip
    // Note: Using gpiochip4 which contains the main GPIO pins on Raspberry Pi 5
    gpio_chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
    if (!gpio_chip) {
        perror("DEV_ModuleInit: Failed to open GPIO chip " GPIO_CHIP_NAME);
        close(spi_fd);
        spi_fd = -1;
        return 1;
    }

    // Configure GPIO lines
    DEV_GPIOConfig(); // This function should also have error checking

    Debug("DEV_ModuleInit: SPI and GPIO initialized successfully.\n");
    return 0;
}

/**
 * @brief Writes a single byte to the SPI bus and reads a byte.
 *
 * This function sends `value` over SPI and stores the received byte (often ignored if only writing).
 * @param value The byte to write to the SPI bus.
 */
void SPI_WriteByte(uint8_t value) {
    uint8_t rx_buffer; // Buffer to store received byte, can be ignored if not needed
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)&value,
        .rx_buf = (unsigned long)&rx_buffer,
        .len = 1,
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = 8,
        // .delay_usecs = 0, // Optional: delay before next transfer
        // .cs_change = 0,   // Optional: keep CS asserted after transfer
    };

    if (spi_fd < 0) {
        fprintf(stderr, "SPI_WriteByte: SPI not initialized.\n");
        return;
    }

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("SPI_WriteByte: Failed to write SPI byte");
    }
}

/**
 * @brief Reads a single byte from the SPI bus.
 *
 * This function sends a dummy byte (0xFF) to generate SCK pulses and reads the byte
 * clocked in from the slave device.
 * @return The byte read from the SPI bus. Returns 0 on error or if SPI not initialized.
 */
UBYTE SPI_ReadByte(void) {
    uint8_t tx_dummy = 0xFF; // Dummy byte to send for reading
    uint8_t rx_buffer = 0;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)&tx_dummy,
        .rx_buf = (unsigned long)&rx_buffer,
        .len = 1,
        .speed_hz = SPI_SPEED_HZ,
        .bits_per_word = 8,
    };

    if (spi_fd < 0) {
        fprintf(stderr, "SPI_ReadByte: SPI not initialized.\n");
        return 0;
    }

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        perror("SPI_ReadByte: Failed to read SPI byte");
        return 0; // Return a defined error value or handle error appropriately
    }
    return rx_buffer;
}

/**
 * @brief Writes a value to a specified GPIO pin.
 * @param pin The GPIO pin number (as defined in DEV_Config.h, e.g., DEV_RST_PIN).
 * @param value The value to write (0 for low, 1 for high).
 */
void DEV_GPIO_Write(int pin, int value) {
    struct gpiod_line *target_line = NULL;

    switch (pin) {
        case DEV_RST_PIN:
            target_line = rst_line;
            break;
        case DEV_CS_PIN:
            target_line = cs_line;
            break;
        case DEV_CS1_PIN:
            target_line = cs1_line;
            break;
        default:
            fprintf(stderr, "DEV_GPIO_Write: Invalid GPIO pin: %d\n", pin);
            return;
    }

    if (target_line) {
        if (gpiod_line_set_value(target_line, value) < 0) {
            perror("DEV_GPIO_Write: Failed to set GPIO line value");
        }
    } else {
        fprintf(stderr, "DEV_GPIO_Write: GPIO pin %d not configured or invalid.\n", pin);
    }
}

/**
 * @brief Reads the value from a specified GPIO pin (currently only DEV_DRDY_PIN).
 * @param pin The GPIO pin number (must be DEV_DRDY_PIN).
 * @return The value of the GPIO pin (0 for low, 1 for high). Returns -1 on error or invalid pin.
 */
int DEV_GPIO_Read(int pin) {
    if (pin == DEV_DRDY_PIN && drdy_line) {
        int value = gpiod_line_get_value(drdy_line);
        if (value < 0) {
            perror("DEV_GPIO_Read: Failed to get GPIO line value");
            return -1; // Indicate error
        }
        return value;
    }
    fprintf(stderr, "DEV_GPIO_Read: Invalid or unconfigured pin for reading: %d\n", pin);
    return -1; // Indicate error or invalid pin
}

/**
 * @brief Cleans up and releases hardware resources.
 *
 * This function releases all requested GPIO lines, closes the GPIO chip handle,
 * and closes the SPI device file descriptor.
 * It should be called when the application is exiting to ensure proper resource management.
 */
void DEV_ModuleExit(void) {
    Debug("DEV_ModuleExit: Releasing SPI and GPIO resources...\n");
    if (rst_line && gpiod_line_is_requested(rst_line)) gpiod_line_release(rst_line);
    if (cs_line && gpiod_line_is_requested(cs_line)) gpiod_line_release(cs_line);
    if (cs1_line && gpiod_line_is_requested(cs1_line)) gpiod_line_release(cs1_line);
    if (drdy_line && gpiod_line_is_requested(drdy_line)) gpiod_line_release(drdy_line);
    
    rst_line = cs_line = cs1_line = drdy_line = NULL; // Nullify pointers after release

    if (gpio_chip) {
        gpiod_chip_close(gpio_chip);
        gpio_chip = NULL;
    }
    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
    }
    Debug("DEV_ModuleExit: SPI and GPIO resources released.\n");
}