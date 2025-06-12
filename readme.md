# ADS1256/DAC8532 Raspberry Pi Driver

A high-performance C library for interfacing with ADS1256 24-bit ADC and DAC8532 16-bit dual-channel DAC on Raspberry Pi systems.

## Features

### ADS1256 ADC Driver
- **24-bit resolution** with up to 30 kSPS sampling rate
- **Programmable Gain Amplifier (PGA)** with gains from 1× to 64×
- **Multiple input modes**: Single-ended (8 channels) and differential (4 pairs)
- **16 selectable data rates**: From 2.5 SPS to 30,000 SPS
- **Performance monitoring**: Real-time efficiency tracking and optimization feedback
- **Optimized multi-channel scanning** with configurable settling times
- **Hardware settling control** for maximum accuracy vs. speed trade-offs

### DAC8532 DAC Driver
- **Dual-channel 16-bit DAC** with individual channel control
- **Rail-to-rail output** (0V to VREF)
- **Voltage-based API** for intuitive control
- **SPI interface** with dedicated chip select

### Hardware Abstraction Layer
- **Raspberry Pi 5 optimized** using modern gpiod library
- **SPI communication** via Linux spidev interface
- **GPIO management** with proper resource cleanup
- **Debug framework** with conditional compilation

## Hardware Requirements

### Supported Platforms
- Raspberry Pi 5 (primary target)
- Raspberry Pi 4/3/Zero (with minor GPIO chip adjustments)

### Pin Connections

| Function | GPIO Pin (BCM) | Physical Pin | Description |
|----------|----------------|--------------|-------------|
| SPI MOSI | GPIO10        | Pin 19       | SPI Data Out |
| SPI MISO | GPIO9         | Pin 21       | SPI Data In |
| SPI SCLK | GPIO11        | Pin 23       | SPI Clock |
| ADC_CS   | GPIO22        | Pin 15       | ADS1256 Chip Select |
| DAC_CS   | GPIO23        | Pin 16       | DAC8532 Chip Select |
| RST      | GPIO18        | Pin 12       | Reset Pin |
| DRDY     | GPIO17        | Pin 11       | Data Ready (ADS1256) |

### Dependencies
```bash
# Install required system packages
sudo apt update
sudo apt install -y build-essential libgpiod-dev

# Enable SPI interface
sudo raspi-config
# Navigate to: Interface Options -> SPI -> Enable
```

## Installation

### Clone and Build
```bash
git clone https://github.com/ZoOtMcNoOt/RaspberryPI5-to-Waveshare-ADDAC
cd RaspberryPI5-to-Waveshare-ADDAC

# Build all examples
cd c/examples/ADS1256_test
make clean && make

cd ../DAC8532_test  
make clean && make

cd ../AD_DA_app
make clean && make

cd ../blink
make clean && make
```

### Quick Test
```bash
# Test GPIO functionality
cd c/examples/blink
sudo ./bin/blink

# Test DAC output
cd ../DAC8532_test
sudo ./bin/dac8532_test

# Test ADC with performance monitoring
cd ../ADS1256_test
sudo ./bin/ads1256_test
```

## Usage Examples

### Basic ADC Reading
```c
#include "ADS1256.h"
#include "DEV_Config.h"

int main() {
    // Initialize hardware
    if (DEV_ModuleInit() != 0) {
        printf("Hardware initialization failed\n");
        return 1;
    }
    
    // Initialize ADS1256: 1000 SPS, Gain=1, Single-ended mode
    if (ADS1256_init(ADS1256_1000SPS, ADS1256_GAIN_1, SCAN_MODE_SINGLE_ENDED) != 0) {
        printf("ADS1256 initialization failed\n");
        return 1;
    }
    
    // Read single channel
    UDOUBLE raw_value = ADS1256_GetChannelValue(0); // Read AIN0
    float voltage = ADS1256_RawToVoltage(raw_value, 5.0f, 0.0f, ADS1256_GAIN_1);
    
    printf("Channel 0: %.4f V\n", voltage);
    
    DEV_ModuleExit();
    return 0;
}
```

### Multi-Channel Optimized Scanning
```c
// Define channels to scan
UBYTE channels[] = {0, 1, 2, 3}; // AIN0, AIN1, AIN2, AIN3
int num_channels = 4;
UDOUBLE adc_values[4];

// Initialize performance monitoring
ADS1256_InitPerformanceMonitoring(ADS1256_1000SPS);

// Optimized scan with 5 settling cycles
ADS1256_GetNChannels_Optimized(adc_values, channels, num_channels, 5);

// Convert and display results
for (int i = 0; i < num_channels; i++) {
    float voltage = ADS1256_RawToVoltage(adc_values[i], 5.0f, 0.0f, ADS1256_GAIN_1);
    printf("Channel %d: %.4f V\n", channels[i], voltage);
}

// Print performance report
ADS1256_PrintPerformanceReport();
```

### DAC Control
```c
#include "DAC8532.h"

// Set DAC outputs
DAC8532_Out_Voltage(DAC8532_CHANNEL_A, 2.5f); // 2.5V on Channel A
DAC8532_Out_Voltage(DAC8532_CHANNEL_B, 1.8f); // 1.8V on Channel B
```

## API Reference

### ADS1256 Functions

#### Initialization
```c
UBYTE ADS1256_init(ADS1256_DRATE drate, ADS1256_GAIN gain, ADS1256_SCAN_MODE scan_mode);
```

#### Data Acquisition
```c
UDOUBLE ADS1256_GetChannelValue(UBYTE Channel);
void ADS1256_GetAllChannels(UDOUBLE *ADC_Value);
void ADS1256_GetNChannels_Optimized(UDOUBLE *ADC_Value, UBYTE *channels, 
                                   UBYTE num_channels, UBYTE settling_cycles);
void ADS1256_GetNChannels_Fast(UDOUBLE *ADC_Value, UBYTE *channels, UBYTE num_channels);
```

#### Utility Functions
```c
float ADS1256_RawToVoltage(UDOUBLE raw_value, float vref_pos, float vref_neg, ADS1256_GAIN gain);
UBYTE ADS1256_ReadChipID(void);
```

#### Performance Monitoring
```c
void ADS1256_InitPerformanceMonitoring(ADS1256_DRATE drate_enum_val);
performance_metrics_t* ADS1256_GetPerformanceMetrics(void);
void ADS1256_PrintPerformanceReport(void);
```

### DAC8532 Functions
```c
void DAC8532_Out_Voltage(UBYTE Channel, float Voltage);
void Write_DAC8532(UBYTE Channel, UWORD Data);
```

### Configuration Enums

#### Data Rates (ADS1256_DRATE)
- `ADS1256_30000SPS` to `ADS1256_2d5SPS`
- Automatic register value lookup via `ADS1256_DRATE_E[]`

#### Gain Settings (ADS1256_GAIN)  
- `ADS1256_GAIN_1` through `ADS1256_GAIN_64`

#### Scan Modes (ADS1256_SCAN_MODE)
- `SCAN_MODE_SINGLE_ENDED`: 8 channels (AIN0-7 vs AINCOM)
- `SCAN_MODE_DIFFERENTIAL_INPUTS`: 4 pairs (AIN0-1, AIN2-3, AIN4-5, AIN6-7)

## Example Applications

### 1. ADS1256 Performance Test (`c/examples/ADS1256_test/`)
Interactive test program featuring:
- **Configurable parameters**: Data rate, gain, channel selection
- **Two scanning modes**: Optimized (full settling) vs Fast (minimal settling)
- **Real-time monitoring**: SPS rates, efficiency percentages, performance status
- **Benchmark comparison**: Side-by-side mode evaluation

**Usage:**
```bash
cd c/examples/ADS1256_test
sudo ./bin/ads1256_test
```

### 2. Combined ADC/DAC Application (`c/examples/AD_DA_app/`)
Demonstrates real-time analog I/O:
- Continuously reads all ADC channels
- Uses ADC Channel 0 to control DAC outputs
- DAC Channel B = ADC Channel 0 voltage
- DAC Channel A = (VREF - ADC Channel 0 voltage)

### 3. DAC Test (`c/examples/DAC8532_test/`)
Simple DAC demonstration with alternating ramp patterns on both channels.

### 4. GPIO Blink (`c/examples/blink/`)
Basic GPIO functionality test using the gpiod library.

## Performance Optimization

### Settling Time vs Speed Trade-offs

**Optimized Mode (`ADS1256_GetNChannels_Optimized`)**:
- Configurable settling cycles (recommended: 3-5)
- Maximum accuracy and noise rejection
- ~70-90% of theoretical SPS efficiency
- Best for precision measurements

**Fast Mode (`ADS1256_GetNChannels_Fast`)**:
- Minimal settling time (1 DRDY cycle)
- Maximum throughput
- ~90-95% of theoretical SPS efficiency  
- Best for high-speed monitoring

### Performance Monitoring Output Example
```
=== ADS1256 Performance Report ===
Runtime: 10.50 seconds
Total Samples Acquired: 8420
Total N-Channel Scan Operations: 2105
Theoretical Max SPS (single channel continuous): 1000
Actual Average Total SPS (all samples / time): 801.9
Overall Efficiency (Effective Per-Channel SPS vs Theoretical): 80.2%
Status: GOOD - Acceptable performance.
```

## Troubleshooting

### Common Issues

**"Failed to open SPI device"**
```bash
# Check SPI is enabled
lsmod | grep spi
# Should show: spi_bcm2835

# Verify device exists
ls -la /dev/spi*
```

**"Failed to open GPIO chip"**
```bash
# Verify gpiod installation
sudo apt install libgpiod-dev libgpiod2

# Check GPIO chip
ls -la /dev/gpiochip*
# For Pi 5: gpiochip4, for Pi 4: gpiochip0
```

**Poor ADC Performance**
- Check reference voltage connections (VREFP, VREFN)
- Verify DRDY pin connection
- Reduce data rate for better accuracy
- Increase settling cycles in optimized mode
- Check for electromagnetic interference

### Debug Mode
Enable debug output by modifying the Makefile:
```makefile
DEBUG = -g -Wall -DDEBUG=1
```

## Contributing

### Code Style
- Follow existing indentation and naming conventions
- Add comprehensive documentation for new functions
- Include example usage in function headers
- Test on actual hardware before submitting

### Adding New Features
1. **New ADC modes**: Extend `ADS1256_SCAN_MODE` enum
2. **Additional data rates**: Update `ADS1256_DRATE` and `ADS1256_DRATE_E[]`
3. **Hardware platforms**: Modify GPIO chip names in `DEV_Config.h`

## Acknowledgments

- Texas Instruments for ADS1256 and DAC8532 documentation
- Raspberry Pi Foundation for hardware platform
- Linux kernel SPI and GPIO subsystem developers
