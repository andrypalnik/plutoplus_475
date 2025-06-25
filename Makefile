CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -static \
    -I. \
    -Ino-OS/include \
    -Ino-OS/drivers/platform/linux \
    -Ino-OS/drivers/api \
    -Ino-OS/drivers/rf-transceiver/ad9361 \
    -Ino-OS/drivers/util \
    -Ino-OS/util \
    -Ino-OS/drivers/clk \
    -Ino-OS/drivers/axi_core/axi_adc_core \
    -Ino-OS/drivers/axi_core/axi_dac_core

CFLAGS += -DAXI_ADC_NOT_PRESENT 


SRC = \
    main.c \
    no-OS/drivers/platform/linux/linux_spi.c \
    no-OS/drivers/platform/linux/linux_gpio.c \
    no-OS/drivers/platform/linux/linux_delay.c \
    no-OS/util/no_os_alloc.c \
    no-OS/drivers/api/no_os_spi.c \
    no-OS/util/no_os_mutex.c \
    no-OS/drivers/rf-transceiver/ad9361/ad9361.c \
    no-OS/drivers/rf-transceiver/ad9361/ad9361_api.c \
    no-OS/drivers/rf-transceiver/ad9361/ad9361_util.c \
    no-OS/drivers/api/no_os_gpio.c \
    no-OS/drivers/platform/linux/linux_delay.c \
    no-OS/util/no_os_util.c \
    no-OS/util/no_os_clk.c \
    no-OS/drivers/axi_core/axi_adc_core/axi_adc_core.c \
    no-OS/drivers/platform/linux/linux_axi_io.c \
	no-OS/drivers/rf-transceiver/ad9361/ad9361_conv.c \
    no-OS/drivers/axi_core/axi_dmac/axi_dmac.c





TARGET = spi_hal_test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(TARGET)
