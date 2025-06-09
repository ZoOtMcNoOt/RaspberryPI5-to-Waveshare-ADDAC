#include "ADS1256.h"

UBYTE ScanMode = 0;

/*
    * function:   Module reset
    * parameter:
    * Info:
    * This function resets the ADS1256 module by toggling the reset pin.
*/

static void ADS1256_reset(void)
{
    DEV_Digital_Write(DEV_RST_PIN, 1);
    DEV_Delay_ms(200);
    DEV_Digital_Write(DEV_RST_PIN, 0);
    DEV_Delay_ms(200);
    DEV_Digital_Write(DEV_RST_PIN, 1);
}

/*
    * function:   send command
    * parameter: 
    *         Cmd: command
    * Info:
    * This function sends a command to the ADS1256 module by writing to the chip select pin and using SPI.
*/

static void ADS1256_WriteCmd(UBYTE Cmd)
{
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(Cmd);
    DEV_Digital_Write(DEV_CS_PIN, 1);
}

/*
    * function:   Write a data to the destination register
    * parameter: 
    *         Reg : Target register
    *         data: Written data
    * Info:
    * This function writes data to a specified register in the ADS1256 module.
*/
static void ADS1256_WriteReg(UBYTE Reg, UBYTE data)
{
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_WREG | Reg);
    DEV_SPI_WriteByte(0x00);
    DEV_SPI_WriteByte(data);
    DEV_Digital_Write(DEV_CS_PIN, 1);
}

/*
    * function:   Read a data from the destination register
    * parameter: 
    *         Reg : Target register
    * Info:
    * This function reads data from a specified register in the ADS1256 module.
    * Returns the read data.
*/

static UBYTE ADS1256_Read_data(UBYTE Reg)
{
    UBYTE temp = 0;
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_RREG | Reg);
    DEV_SPI_WriteByte(0x00);
    DEV_Delay_ms(1); // Allow time for the read operation to complete
    temp = DEV_SPI_ReadByte();
    DEV_Digital_Write(DEV_CS_PIN, 1);
    return temp;
}

/*
    * function: Wait for the Data Ready (DRDY) signal
    * parameter: None
    * Info:
    * This function waits for the DRDY pin to go low, indicating that data is ready to be read.
    * If the DRDY pin does not go low within a specified timeout period, it prints a timeout message.
*/

static void ADS1256_WaitDRDY(void)
{
    UDOUBLE i = 0;
    for(i = 0; i< 4000000; i++) {
        if(DEV_Digital_Read(DEV_DRDY_PIN) == 0) {
            break;
        }
    }
    if (i >= 4000000) {
        printf("ADS1256 Time Out! ...\n");
    }
}

/*
    * function: Read the chip ID
    * parameter: None
    * Info:
    * This function reads the chip ID from the ADS1256 module.
    * Returns the chip ID.
*/
UBYTE ADS1256_ReadChipID(void)
{
    UBYTE id;
    ADS1256_WaitDRDY(); // Wait for DRDY to go low before reading
    id = ADS1256_Read_data(REG_STATUS); 
    return id>>4; // Return the chip ID by shifting right 4 bits
}

void ADS1256_ConfigADC(ADS1256_GAIN gain, ADS1256_DRATE drate)
{
    ADS1256_WaitDRDY(); // Wait for DRDY to go low before configuring
    UBYTE buf[4] = {0, 0, 0, 0};

    buf[0] = (0<<3) | (1<<2) | (1<<1); // Set the ADC to normal mode, gain 1, and continuous conversion
    buf[1] = 0x08 ; // Set the ADC to 8 channels, AIN0 as the input channel
    buf[2] = (0<<5) | (0<<3) | (gain<<0); // Set the gain in the ADCON register
    buf[3] = ADS1256_DRATE_E[drate]; // Set the data rate in the DRATE register
    

}