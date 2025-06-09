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

    buf[0] = (0<<3) | (1<<2) | (0<<1); // Set the ADC to normal mode, gain 1, and continuous conversion
    buf[1] = 0x08 ; // Set the ADC to 8 channels, AIN0 as the input channel
    buf[2] = (0<<5) | (0<<3) | (gain<<0); // Set the gain in the ADCON register
    buf[3] = ADS1256_DRATE_E[drate]; // Set the data rate in the DRATE register
    
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_WREG | 0);
    DEV_SPI_WriteByte(0x03);

    DEV_SPI_WriteByte(buf[0]);
    DEV_SPI_WriteByte(buf[1]);
    DEV_SPI_WriteByte(buf[2]);
    DEV_SPI_WriteByte(buf[3]);
    DEV_Digital_Write(DEV_CS_PIN, 1);
    DEV_Delay_ms(1); // Allow time for the configuration to take effect
}

static void ADS1256_SetChannel(UBYTE Channel) {
    if (Channel > 7) {
        return;
    }
    ADS1256_WriteReg(REG_MUX, (Channel << 4) | (1<<3)); // Set the channel in the MUX register
}

static void ADS1256_SetDiffChannal(UBYTE Channal) {
    if (Channal == 0){
        ADS1256_WriteReg(REG_MUX, (0 << 4) | 1); // Set differential channel AIN0-AIN1
    } else if (Channal == 1) {
        ADS1256_WriteReg(REG_MUX, (2 << 4) | 3); // Set differential channel AIN2-AIN3
    } else if (Channal == 2) {
        ADS1256_WriteReg(REG_MUX, (4 << 4) | 5); // Set differential channel AIN4-AIN5
    } else if (Channal == 3) {
        ADS1256_WriteReg(REG_MUX, (6 << 4) | 7); // Set differential channel AIN6-AIN7
    }
}

void ADS1256_SetMode(UBYTE Mode) {
    if (Mode == 0) {
        ScanMode = 0; // Single-ended input mode
    } else if (Mode == 1) {
        ScanMode = 1; // Differential input mode
    }
}

UBYTE ADS1256_init(void) {
    ADS1256_reset();
    if(ADS1256_ReadChipID() == 3) {
        printf("ID Read success \r\n");
    } else {
        printf("ID Read failed \r\n");
        return 1; // Initialization failed
    }
    ADS1256_ConfigADC(ADS1256_GAIN_1, ADS1256_30000SPS); // Configure the ADC with gain 1 and data rate 30k SPS
    return 0;
}

static UDOUBLE ADS1256_read_ADC_Data(void) {
    UDOUBLE read =0;
    UBYTE buf[3] = {0, 0, 0};

    ADS1256_WaitDRDY(); // Wait for DRDY to go low before reading
    DEV_Digital_Write(DEV_CS_PIN, 0);
    DEV_SPI_WriteByte(CMD_RDATA); // Send the read data command
    buf[0] = DEV_SPI_ReadByte(); // Read the first byte
    buf[1] = DEV_SPI_ReadByte(); // Read the second byte
    buf[2] = DEV_SPI_ReadByte(); // Read the third byte
    DEV_Digital_Write(DEV_CS_PIN, 1); // Deselect the chip
    read = ((UDOUBLE)buf[0] << 16) & 0x00FF0000; // Combine the bytes into a single value
    read |= ((UDOUBLE)buf[1] << 8);  // Combine the second byte
    read |= buf[2]; // Combine the third byte
    if (read & 0x800000) read &= 0xFF000000;
    return read; // Return the combined value
}

UDOUBLE ADS1256_GetChannalValue(UBYTE Channel) {
    UDOUBLE Value = 0;
    while(DEV_Digital_Read(DEV_DRDY_PIN) == 1); // Wait for DRDY to go low
    if(ScanMode == 0) { // Single-ended input mode (8 channels  )
        if(Channel >= 8) {
            return 0; // Invalid channel
        }
        ADS1256_SetChannel(Channel); // Set the channel
        ADS1256_WriteCmd(CMD_SYNC); // Synchronize the ADC
        ADS1256_WriteCmd(CMD_WAKEUP); // Wake up the ADC
        Value = ADS1256_read_ADC_Data(); // Read the ADC data
    } else { // Differential input mode (4 channels)
        if(Channel >= 4) {
            return 0; // Invalid channel
        }
        ADS1256_SetDiffChannal(Channel); // Set the differential channel
        ADS1256_WriteCmd(CMD_SYNC); // Synchronize the ADC
        ADS1256_WriteCmd(CMD_WAKEUP); // Wake up the ADC
        Value = ADS1256_read_ADC_Data(); // Read the ADC data
    }
    return Value; // Return the read value
}

void ADS1256_GetAll(UDOUBLE *ADC_Value) {
    UBYTE i;
    
    // Use continuous read mode for faster sampling
    ADS1256_WriteCmd(CMD_RDATAC); // Start continuous read mode
    
    for(i=0; i<8; i++) {
        ADS1256_SetChannel(i); // Set the channel
        ADS1256_WriteCmd(CMD_SYNC); // Synchronize the ADC
        ADS1256_WriteCmd(CMD_WAKEUP); // Wake up the ADC
        
        // Wait for conversion to complete
        while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
        
        // Read the data directly without sending CMD_RDATA (continuous mode)
        UBYTE buf[3];
        DEV_Digital_Write(DEV_CS_PIN, 0);
        buf[0] = DEV_SPI_ReadByte();
        buf[1] = DEV_SPI_ReadByte();
        buf[2] = DEV_SPI_ReadByte();
        DEV_Digital_Write(DEV_CS_PIN, 1);
        
        UDOUBLE read = ((UDOUBLE)buf[0] << 16) & 0x00FF0000;
        read |= ((UDOUBLE)buf[1] << 8);
        read |= buf[2];
        if (read & 0x800000) read &= 0xFF000000;
        
        ADC_Value[i] = read;
    }
    
    ADS1256_WriteCmd(CMD_SDATAC); // Stop continuous read mode
}

// Fast optimized version of GetAll that minimizes overhead
void ADS1256_GetAll_Fast(UDOUBLE *ADC_Value) {
    UBYTE i;
    UBYTE buf[3];
    UDOUBLE read;
    
    for(i=0; i<8; i++) {
        // Set channel efficiently
        DEV_Digital_Write(DEV_CS_PIN, 0);
        DEV_SPI_WriteByte(CMD_WREG | REG_MUX);
        DEV_SPI_WriteByte(0x00);
        DEV_SPI_WriteByte((i << 4) | (1<<3));
        DEV_Digital_Write(DEV_CS_PIN, 1);
        
        // Sync and wakeup in one transaction
        DEV_Digital_Write(DEV_CS_PIN, 0);
        DEV_SPI_WriteByte(CMD_SYNC);
        DEV_SPI_WriteByte(CMD_WAKEUP);
        DEV_Digital_Write(DEV_CS_PIN, 1);
        
        // Wait for conversion
        while(DEV_Digital_Read(DEV_DRDY_PIN) == 1);
        
        // Read data
        DEV_Digital_Write(DEV_CS_PIN, 0);
        DEV_SPI_WriteByte(CMD_RDATA);
        buf[0] = DEV_SPI_ReadByte();
        buf[1] = DEV_SPI_ReadByte();
        buf[2] = DEV_SPI_ReadByte();
        DEV_Digital_Write(DEV_CS_PIN, 1);
        
        read = ((UDOUBLE)buf[0] << 16) & 0x00FF0000;
        read |= ((UDOUBLE)buf[1] << 8);
        read |= buf[2];
        if (read & 0x800000) read &= 0xFF000000;
        
        ADC_Value[i] = read;
    }
}