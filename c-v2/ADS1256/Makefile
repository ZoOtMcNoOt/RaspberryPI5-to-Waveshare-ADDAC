# Makefile for ADS1256 driver using libgpiod (Raspberry Pi 5 compatible)

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
LIBS = -lgpiod
TARGET = ads1256
TEST_TARGET = test_ads1256
SOURCE = ads1256.c
TEST_SOURCE = test_ads1256.c
HEADER = ads1256.h

# Default target
all: $(TARGET) $(TEST_TARGET)

# Build the main executable
$(TARGET): $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

# Build the test executable
$(TEST_TARGET): $(TEST_SOURCE) $(SOURCE) $(HEADER)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SOURCE) $(SOURCE) $(LIBS)

# Install libgpiod if not present (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libgpiod-dev libgpiod2

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET)

# Run main program with default parameters (10 measurements)
run: $(TARGET)
	sudo ./$(TARGET) 10

# Run test program
test: $(TEST_TARGET)
	sudo ./$(TEST_TARGET)

# Debug build with additional flags
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET) $(TEST_TARGET)

# Check if running on Raspberry Pi 5
check-rpi5:
	@echo "Checking Raspberry Pi version..."
	@cat /proc/cpuinfo | grep "Revision" | grep -q "d04170" && echo "Raspberry Pi 5 detected" || echo "Not RPi 5 - may need GPIO chip adjustment"

# Install and build everything
setup: install-deps all

.PHONY: all clean run test debug check-rpi5 install-deps setup