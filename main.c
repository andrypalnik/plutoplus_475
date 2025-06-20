#include "main.h"

struct no_os_spi_init_param spi_param = {
	.device_id = 1,
	.max_speed_hz = 10000000,
	.mode = NO_OS_SPI_MODE_0,
	.chip_select = 0,
	.platform_ops = &linux_spi_ops,
	.extra = "/dev/spidev1.0"
};

#if 1
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

uint32_t fpga_read_reg(off_t phys_addr);

uint32_t fpga_read_reg(off_t phys_addr)
{
    int mem_fd;
    void *map_base, *virt_addr;
    uint32_t read_result;

    // Відкриваємо /dev/mem
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("open /dev/mem");
        exit(EXIT_FAILURE);
    }

    // Відображаємо сторінку пам’яті, що містить потрібну фізичну адресу
    map_base = mmap(
        NULL,
        MAP_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        mem_fd,
        phys_addr & ~MAP_MASK
    );

    if (map_base == MAP_FAILED) {
        perror("mmap");
        close(mem_fd);
        exit(EXIT_FAILURE);
    }

    // Віртуальна адреса потрібного регістра
    virt_addr = map_base + (phys_addr & MAP_MASK);

    // Зчитування 32-бітного значення
    read_result = *((volatile uint32_t *)virt_addr);

    // Очищення
    munmap(map_base, MAP_SIZE);
    close(mem_fd);

    return read_result;
}

void scan_frequencies(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr)
{
    struct rf_rssi rssi;
    uint32_t value;
    int32_t gain;
	const int delay = 10000;
	int circle_time;

    for (uint16_t i = 0; i < count; i++) {
         int freq_mhz = freqs[i];
         uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;

        //uint64_t freq_hz = (uint64_t)1080000000ULL;

        // Встановлюємо RX частоту
        ad9361_set_rx_lo_freq(phy, freq_hz);

        // Очікуємо стабілізацію AGC / RSSI
        usleep(delay); // 5 мс

	

        // Зчитуємо значення з FPGA-регістра
        value = fpga_read_reg(reg_addr);

	    

        if (value > 50) {
			printf("Зчитано з 0x%lX: %d\n", (unsigned long)reg_addr, value);
            printf("\n[!] Детекція сигналу на частоті %d МГц (value = %u)\n", freq_mhz, value);

			ad9361_get_rx_rf_gain(phy, 0, &gain);
			printf("Gain: %d dB\n", gain);
        }
    }

	circle_time = ((delay*count)/1000);

	printf("time spent for one circle is %d ms\n", circle_time);
}
#endif



// int main() {
int main(int argc, char *argv[]) {
    printf("main start\n");
    // struct no_os_gpio_desc *gpio_resetb;
    // printf("testpoint 1\n");
    // struct no_os_gpio_init_param gpio_param = {
    //     .number = 973,  // заміни на свій GPIO
    //     .platform_ops = &linux_gpio_ops,
    //     .extra = "/dev/gpiochip0"
    // };

    // printf("testpoint 2\n");
    // if (no_os_gpio_get(&gpio_resetb, &gpio_param) < 0 ||
    //     no_os_gpio_direction_output(gpio_resetb, NO_OS_GPIO_HIGH) < 0) {
    //     printf("GPIO init failed\n");
    //     return -1;
    // }

    // printf("testpoint 3\n");


    // printf("testpoint 4\n");
    struct no_os_spi_desc *spi_desc;
    if (no_os_spi_init(&spi_desc, &spi_param) < 0) {
        printf("SPI init failed\n");
    return -1;
    }

    // printf("testpoint 5\n");

    // int id = ad9361_spi_read(spi_desc, REG_PRODUCT_ID); 
    // if (id < 0)
    //     printf("Помилка зчитування ID\n");
    // else
    //     printf("Product ID: 0x%02X\n", id);


    printf("testpoint 1\n");


    default_init_param.gpio_resetb.number = 973;
    default_init_param.gpio_sync.number = -1;
    default_init_param.gpio_cal_sw1.number = -1;
    default_init_param.gpio_cal_sw2.number = -1;
 



    struct ad9361_rf_phy *ad9361_phy;
    // ad9361_init(&ad9361_phy, &default_init_param);
    int ret = ad9361_init(&ad9361_phy, &default_init_param);
    if (ret < 0) {
        printf("ad9361_init failed: %d\n", ret);
        return -1;
    }

    // ad9361_spi_write(spi_desc, 0x010, 0xC0); // conf_1: default DDR
    // ad9361_spi_write(spi_desc, 0x011, 0x00); // conf_2: default
    // ad9361_spi_write(spi_desc, 0x012, 0x01); // conf_3: full port


    ad9361_set_tx_fir_config(ad9361_phy, tx_fir_config);
    ad9361_set_rx_fir_config(ad9361_phy, rx_fir_config);
 
    // ad9361_phy->pdata->ensm_pin_ctrl = false;
    // ad9361_phy->pdata->ensm_pin_pulse_mode = false; 

    // ad9361_set_en_state_machine_mode(ad9361_phy, ENSM_MODE_FDD);
    // usleep(10000);
    // ad9361_ensm_set_state(ad9361_phy, ENSM_STATE_ALERT, true); // use_spi = true
    // usleep(10000);
    // ad9361_ensm_set_state(ad9361_phy, ENSM_STATE_TX, true);    // use_spi = true
 


    // ad9361_bist_prbs(ad9361_phy, BIST_INJ_TX);

    // uint32_t val = ad9361_spi_read(spi_desc, REG_BIST_CONFIG);
    // printf("BIST CONFIG REG: 0x%08X\n", val);

    // uint32_t ensm_state = ad9361_spi_read(spi_desc, REG_STATE);
    // printf("ENSM state: 0x%02X\n", ensm_state);

    uint8_t port_conf = ad9361_spi_read(spi_desc, REG_PARALLEL_PORT_CONF_1); 
    printf("REG_PARALLEL_PORT_CONF_1: 0x%02X\n", port_conf);
    port_conf = ad9361_spi_read(spi_desc, REG_PARALLEL_PORT_CONF_2); 
    printf("REG_PARALLEL_PORT_CONF_2: 0x%02X\n", port_conf);
    port_conf = ad9361_spi_read(spi_desc, REG_PARALLEL_PORT_CONF_3); 
    printf("REG_PARALLEL_PORT_CONF_3: 0x%02X\n", port_conf);

    // ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_MGC);
	// ad9361_set_rx_rf_gain(ad9361_phy, 0, 0);

    // ad9361_set_rx_gain_control_mode(ad9361_phy, 1, RF_GAIN_MGC);
	// ad9361_set_rx_rf_gain(ad9361_phy, 1, 0);

	// ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_SLOWATTACK_AGC);
	// ad9361_set_rx_gain_control_mode(ad9361_phy, 1, RF_GAIN_SLOWATTACK_AGC);

	ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_FASTATTACK_AGC);
	ad9361_set_rx_gain_control_mode(ad9361_phy, 1, RF_GAIN_FASTATTACK_AGC);

	if (argc >= 2) {
    	uint64_t rx_freq = strtoull(argv[1], NULL, 10); // частота в Гц 
    	ad9361_set_rx_lo_freq(ad9361_phy, rx_freq);
    	printf("Set RX & TX frequency: %llu Hz\n", rx_freq);
	}
	
	if (argc >= 3) {
    	ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_MGC);
		int32_t rx_gain = (int32_t)strtol(argv[2], NULL, 10); // дБ
    	ad9361_set_rx_rf_gain(ad9361_phy, 0, rx_gain);
    	printf("Set RX0 GAIN: %d DB\n", rx_gain);
	}

    ad9361_set_rx_rf_bandwidth(ad9361_phy, 5000000); // 10 МГц
	ad9361_set_rx_sampling_freq(ad9361_phy, 30000000);

    sleep(1);

    struct rf_rssi rssi;
    int32_t gain;

	uint16_t freqs_amount = sizeof(freqs) / sizeof(freqs[0]);
    printf("Кількість частот: %d\n", freqs_amount);

    while (true)
    {
        // if (ad9361_get_rx_rssi(ad9361_phy, 0, &rssi) == 0)
        //     printf("RX0 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
        //     // printf("RX0 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
        // else
        //     printf("Failed to read RX0 RSSI\n");

        // ad9361_get_rx_rf_gain(ad9361_phy, 0, &gain);
        // printf("Gain: %d dB\n", gain);
 
        // if (ad9361_get_rx_rssi(ad9361_phy, 1, &rssi) == 0)
        //     printf("RX1 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
        //     // printf("RX1 RSSI: %.1f dB\n", (double)rssi.symbol);
        //     // printf("RX1 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
        // else
        //     printf("Failed to read RX1 RSSI\n");

        // ad9361_get_rx_rf_gain(ad9361_phy, 1, &gain);
        // printf("Gain: %d dB\n", gain);

		// off_t my_register = 0x43C00000;  // заміни на свою адресу
    	// uint32_t value = fpga_read_reg(my_register);
    	// //printf("Зчитано з 0x%lX: 0x%08X\n", (unsigned long)my_register, value);
		// printf("Зчитано з 0x%lX: %d\n", (unsigned long)my_register, value);

        // printf("while\n");

		off_t my_register = 0x43C00000;
		scan_frequencies(ad9361_phy, freqs, freqs_amount, my_register);
        //sleep(1);
    }

    sleep(1);

}