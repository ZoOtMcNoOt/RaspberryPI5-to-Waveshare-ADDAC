# Define pin constants
RST_PIN = 18
CS_PIN = 22
CS_DAC_PIN = 23
DRDY_PIN = 17

try:
    import lgpio
    import time
    import spidev

    # GPIO shim for lgpio
    class GPIOWrapper:
        BCM = None
        OUT = "out"
        IN = "in"
        HIGH = 1
        LOW = 0

        def __init__(self):
            self.handle = lgpio.gpiochip_open(0)
            self.claimed = {}

        def setmode(self, mode):
            pass

        def setup(self, pin, mode):
            try:
                if mode == self.OUT:
                    lgpio.gpio_claim_output(self.handle, pin, 0)
                else:
                    lgpio.gpio_claim_input(self.handle, pin)
            except lgpio.error:
                if pin in self.claimed:
                    try:
                        lgpio.gpio_free(self.handle, pin)
                    except lgpio.error:
                        pass
                # Try again
                if mode == self.OUT:
                    lgpio.gpio_claim_output(self.handle, pin, 0)
                else:
                    lgpio.gpio_claim_input(self.handle, pin)
            self.claimed[pin] = mode

        def output(self, pin, value):
            lgpio.gpio_write(self.handle, pin, value)

        def input(self, pin):
            return lgpio.gpio_read(self.handle, pin)

        def cleanup(self):
            for pin in list(self.claimed.keys()):
                try:
                    lgpio.gpio_free(self.handle, pin)
                    del self.claimed[pin]
                except lgpio.error:
                    pass
            lgpio.gpiochip_close(self.handle)

    GPIO = GPIOWrapper()

    # SPI setup
    spi = spidev.SpiDev()
    spi.open(0, 0)
    spi.max_speed_hz = 1000000  # 1.0 MHz
    spi.mode = 0

    def module_init():
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(RST_PIN, GPIO.OUT)
        GPIO.setup(CS_PIN, GPIO.OUT)
        GPIO.setup(CS_DAC_PIN, GPIO.OUT)
        GPIO.setup(DRDY_PIN, GPIO.IN)
        return 0

    def digital_write(pin, value):
        GPIO.output(pin, value)

    def digital_read(pin):
        return GPIO.input(pin)

    def delay_ms(milliseconds):
        time.sleep(milliseconds / 1000.0)

    def spi_writebyte(data):
        spi.xfer2(data)

    def spi_readbytes(length):
        result = spi.readbytes(length)
        return result

    def destroy():
        GPIO.cleanup()
        spi.close()

except Exception as e_config:
    print(f"CRITICAL ERROR during config_patched.py loading: {e_config}")
    raise

