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

#define START_FREQ   600
#define STOP_FREQ    5950  

void scan_frequencies(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr)
{
    struct rf_rssi rssi;
    uint32_t value;
    int32_t gain;
	const int delay = 5000;
	int circle_time;

    for (uint16_t i = START_FREQ; i < STOP_FREQ; i+=1) {
         int freq_mhz = i;
         uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;

        //uint64_t freq_hz = (uint64_t)1080000000ULL;

        // Встановлюємо RX частоту
        ad9361_set_rx_lo_freq(phy, freq_hz);

        // Очікуємо стабілізацію AGC / RSSI
        usleep(delay); // 5 мс


        // Зчитуємо значення з FPGA-регістра
        value = fpga_read_reg(reg_addr);

        if (ad9361_get_rx_rssi(phy, 0, &rssi) == 0)
            printf("RX0 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
            // printf("RX0 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
        else
            printf("Failed to read RX0 RSSI\n");

        ad9361_get_rx_rf_gain(phy, 0, &gain);
		printf("Gain: %d dB\n", gain);

        printf("Зчитано з 0x%lX: %d\n", (unsigned long)reg_addr, value);
        printf("\n[!] Детекція сигналу на частоті %d МГц (value = %u)\n", freq_mhz, value);
	    

        // if (value > 50) {
		// 	printf("Зчитано з 0x%lX: %d\n", (unsigned long)reg_addr, value);
        //     printf("\n[!] Детекція сигналу на частоті %d МГц (value = %u)\n", freq_mhz, value);

		// 	//ad9361_get_rx_rf_gain(phy, 0, &gain);
		// 	//printf("Gain: %d dB\n", gain);
        // }
    }

	circle_time = ((delay*(STOP_FREQ - START_FREQ))/1000);

	printf("time spent for one circle is %d ms\n", circle_time);
}
#endif

#define RSSI_OFFSET_DBM -41.7  // Емпірично підібраний offset

void scan_frequencies_for_manual_mode(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr)
{
    struct rf_rssi rssi;
    uint32_t value;
    int32_t gain;
    const int delay = 5000;
    int circle_time;

    FILE *f = fopen("/tmp/rssi_scan.csv", "w");
    if (!f) {
        perror("Не вдалося відкрити файл для запису");
        return;
    }
    fprintf(f, "Frequency_MHz,RSSI_dBm,G_gain_dB,FPGA_value\n");

    for (uint16_t i = START_FREQ; i <= STOP_FREQ; i += 1) {
        int freq_mhz = i;
        uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;

        ad9361_set_rx_lo_freq(phy, freq_hz);
        usleep(delay); // стабілізація

        value = fpga_read_reg(reg_addr);

        if (ad9361_get_rx_rssi(phy, 0, &rssi) == 0) {
            ad9361_get_rx_rf_gain(phy, 0, &gain);

            //double rssi_dbm;
            double rssi_dbm = (rssi.symbol > 0)
                ? 10.0 * log10((double)rssi.symbol) + RSSI_OFFSET_DBM
                : -120.0;

            printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
            printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
            printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
                rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);
            printf("Gain: %d dB, RSSI ≈ %.2f dBm\n\n", gain, rssi_dbm);

            fprintf(f, "%d,%.2f,%d,%u\n", freq_mhz, rssi_dbm, gain, value);
        } else {
            printf("Не вдалося зчитати RSSI\n");
        }
    }

    fclose(f);
    circle_time = ((delay * (STOP_FREQ - START_FREQ)) / 1000);
    printf("time spent for one circle is %d ms\n", circle_time);
}

void scan_frequencies_2(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr)
{
    struct rf_rssi rssi;
    uint32_t value;
    int32_t gain;
    const int delay = 5000;
    int circle_time;

    FILE *f = fopen("/tmp/rssi_scan.csv", "w");
    if (!f) {
        perror("Не вдалося відкрити файл для запису");
        return;
    }
    fprintf(f, "Frequency_MHz,RSSI_dBm,G_gain_dB,FPGA_value\n");

    // Буфер для зібраних частот
    typedef struct {
        int freq_mhz;
        uint32_t symbol;
        uint32_t preamble;
        uint32_t fpga_value;
    } rssi_data_t;

    rssi_data_t results[STOP_FREQ - START_FREQ + 1];
    int collected = 0;
    bool collecting = false;

    for (uint16_t i = START_FREQ; i <= STOP_FREQ; i += 1) {
        int freq_mhz = i;
        uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;

        ad9361_set_rx_lo_freq(phy, freq_hz);
        usleep(delay); // стабілізація

        value = fpga_read_reg(reg_addr);

        if (value >= 0) {
            collecting = true;

            if (ad9361_get_rx_rssi(phy, 0, &rssi) == 0) {
                

                double rssi_dbm = (rssi.symbol > 0)
                    ? 10.0 * log10((double)rssi.symbol) + RSSI_OFFSET_DBM
                    : -120.0;
                
                ad9361_get_rx_rf_gain(phy, 0, &gain);

                // printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
                // printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
                // printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
                // rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);

                printf("Gain: %d dB, RSSI ≈ %.2f dBm\n\n", gain, rssi_dbm);
                fprintf(f, "%d,%.2f,%d,%u\n", freq_mhz, rssi_dbm, gain, value);

                results[collected++] = (rssi_data_t){
                    .freq_mhz = freq_mhz,
                    .symbol = rssi.symbol,
                    .preamble = rssi.preamble,
                    .fpga_value = value
                };
            }
        } else if (collecting) {
            // Якщо раніше йшло збирання, і тепер value == 0 — завершити
            break;
        }
        
        ad9361_get_rx_rf_gain(phy, 0, &gain);

        printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
        printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
        printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
        rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);
    }

    fclose(f);
    circle_time = ((delay * (STOP_FREQ - START_FREQ)) / 1000);
    printf("time spent for one circle is %d ms\n", circle_time);

    // Аналіз зібраного: знайти мінімальне abs(symbol - preamble)
    if (collected > 0) {
        int min_diff = abs((int)results[0].symbol - (int)results[0].preamble);
        int best_freq = results[0].freq_mhz;

        for (int i = 1; i < collected; i++) {
            int diff = abs((int)results[i].symbol - (int)results[i].preamble);
            if (diff > min_diff) {
                min_diff = diff;
                best_freq = results[i].freq_mhz;
            }
        }

        printf(">>> Виявлено центральну частоту: %d МГц (max. |symbol - preamble| = %d)\n", best_freq, min_diff);
    } else {
        printf("Не виявлено жодного сигналу з FPGA регістром > 0\n");
    }
}


void scan_frequencies_3(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr)
{
    struct rf_rssi rssi;
    uint32_t value;
    int32_t gain;
    const int delay = 5000;
    int circle_time;

    FILE *f = fopen("/tmp/rssi_scan.csv", "w");
    if (!f) {
        perror("Не вдалося відкрити файл для запису");
        return;
    }
    fprintf(f, "Frequency_MHz,RSSI_dBm,G_gain_dB,FPGA_value\n");

    typedef struct {
        int freq_mhz;
        uint32_t symbol;
        uint32_t preamble;
        uint32_t fpga_value;
    } rssi_data_t;

    rssi_data_t *cluster = malloc(sizeof(rssi_data_t) * (STOP_FREQ - START_FREQ + 1));
    if (!cluster) {
        perror("malloc");
        fclose(f);
        return;
    }
    int cluster_size = 0;
    bool collecting = false;

    for (uint16_t i = START_FREQ; i <= STOP_FREQ; i += 1) {
        int freq_mhz = i;
        uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;

        ad9361_set_rx_lo_freq(phy, freq_hz);
        usleep(delay);

        value = fpga_read_reg(reg_addr);

        if (value > 50) {
            collecting = true;

            if (ad9361_get_rx_rssi(phy, 0, &rssi) == 0) {
                ad9361_get_rx_rf_gain(phy, 0, &gain);

                double rssi_dbm = (rssi.symbol > 0)
                    ? 10.0 * log10((double)rssi.symbol) + RSSI_OFFSET_DBM
                    : -120.0;

                //  printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
                //  printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
                //  printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
                //      rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);
                // printf("Gain: %d dB, RSSI ≈ %.2f dBm\n\n", gain, rssi_dbm);

                fprintf(f, "%d,%.2f,%d,%u\n", freq_mhz, rssi_dbm, gain, value);

                cluster[cluster_size++] = (rssi_data_t){
                    .freq_mhz = freq_mhz,
                    .symbol = rssi.symbol,
                    .preamble = rssi.preamble,
                    .fpga_value = value
                };
            }
        } else if (collecting && cluster_size > 0) {
            // Обробка зібраного кластеру
            int min_diff = abs((int)cluster[0].symbol - (int)cluster[0].preamble);
            int best_freq = cluster[0].freq_mhz;

            for (int j = 1; j < cluster_size; j++) {
                int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                if (diff > min_diff) {
                    min_diff = diff;
                    best_freq = cluster[j].freq_mhz;
                }
            }

            // ad9361_get_rx_rf_gain(phy, 0, &gain);

            // printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
            // printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
            // printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
            // rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);

            printf(">>> Виявлено центральну частоту: %d МГц (max. |symbol - preamble| = %d)\n\n", best_freq, min_diff);

            // Готуємось до нового кластеру
            cluster_size = 0;
            collecting = false;
        }

        // ad9361_get_rx_rf_gain(phy, 0, &gain);

        // printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
        // printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
        // printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
        // rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);
    }

    // Якщо завершили цикл, але ще був активний кластер — обробити
    if (collecting && cluster_size > 0) {
        int min_diff = abs((int)cluster[0].symbol - (int)cluster[0].preamble);
        int best_freq = cluster[0].freq_mhz;

        for (int j = 1; j < cluster_size; j++) {
            int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
            if (diff > min_diff) {
                min_diff = diff;
                best_freq = cluster[j].freq_mhz;
            }
        }

        printf(">>> Виявлено центральну частоту: %d МГц (max. |symbol - preamble| = %d)\n\n", best_freq, min_diff);
    }

    free(cluster);
    fclose(f);
    circle_time = ((delay * (STOP_FREQ - START_FREQ)) / 1000);
    printf("time spent for one circle is %d ms\n", circle_time);
}

#define MAX_SILENT_STEPS 3
int silent_counter = 0;

void scan_frequencies_4(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr)
{
    struct rf_rssi rssi;
    uint32_t value;
    int32_t gain;
    const int delay = 5000;
    int circle_time;

    FILE *f = fopen("/tmp/rssi_scan.csv", "w");
    if (!f) {
        perror("Не вдалося відкрити файл для запису");
        return;
    }
    fprintf(f, "Frequency_MHz,RSSI_dBm,G_gain_dB,FPGA_value\n");

    typedef struct {
        int freq_mhz;
        uint32_t symbol;
        uint32_t preamble;
        uint32_t fpga_value;
        int32_t gain;
    } rssi_data_t;

    rssi_data_t *cluster = malloc(sizeof(rssi_data_t) * (STOP_FREQ - START_FREQ + 1));
    if (!cluster) {
        perror("malloc");
        fclose(f);
        return;
    }
    int cluster_size = 0;
    bool collecting = false;

    for (uint16_t i = START_FREQ; i <= STOP_FREQ; i += 1) {
        int freq_mhz = i;
        uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;

        ad9361_set_rx_lo_freq(phy, freq_hz);
        usleep(delay);

        value = fpga_read_reg(reg_addr);

        if (value > 50) {

            collecting = true;
            silent_counter = 0; // обнуляємо мовчання

            if (ad9361_get_rx_rssi(phy, 0, &rssi) == 0) {
                ad9361_get_rx_rf_gain(phy, 0, &gain);

                double rssi_dbm = (rssi.symbol > 0)
                    ? 10.0 * log10((double)rssi.symbol) + RSSI_OFFSET_DBM
                    : -120.0;

                //  printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
                //  printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
                //  printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
                //      rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);
                // printf("Gain: %d dB, RSSI ≈ %.2f dBm\n\n", gain, rssi_dbm);

                fprintf(f, "%d,%.2f,%d,%u\n", freq_mhz, rssi_dbm, gain, value);

                cluster[cluster_size++] = (rssi_data_t){
                    .freq_mhz = freq_mhz,
                    .symbol = rssi.symbol,
                    .preamble = rssi.preamble,
                    .fpga_value = value,
                    .gain = gain
                };
            }
        } else if (collecting && cluster_size > 0) {

            silent_counter++;

            if (silent_counter >= MAX_SILENT_STEPS && cluster_size > 0) {
                // // Завершуємо кластер лише після N тиші
                // int min_diff = abs((int)cluster[0].symbol - (int)cluster[0].preamble);
                // int best_freq = cluster[0].freq_mhz;
                // uint32_t best_value = cluster[0].fpga_value;

                // for (int j = 1; j < cluster_size; j++) {
                //     int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                //     if (diff > min_diff) {
                //         min_diff = diff;
                //         best_freq = cluster[j].freq_mhz;
                //     }

                //     printf("[!] Детекція сигналу на частоті %d МГц\n", cluster[j].freq_mhz);
                //     printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, cluster[j].fpga_value);
                //     printf("RSSI.symbol = %u, preamble = %u, Gain: %d dB, diff  = %d\n",
                //         cluster[j].symbol, cluster[j].preamble, cluster[j].gain, diff);
                // }

                // printf(">>> Виявлено центральну частоту: %d МГц (max. |symbol - preamble| = %d)\n\n", best_freq, min_diff);


                int best_freq = -1;
                int best_score = -1;

                // for (int j = 0; j < cluster_size; j++) {
                //     int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                //     uint32_t fpga = cluster[j].fpga_value;

                //     // Евристика: чим менше diff і більше fpga — тим краще
                //     //int score = (int)fpga - diff; // коефіцієнт можна підібрати

                //     //int score = (int)fpga - diff / 10 + gain * 2;
                    
                //     // Чим менше diff, більше fpga, і нижчий gain (бо AGC бачить потужний сигнал), тим краще
                //     int score = (int)fpga * 2 - diff / 10 + (50 - cluster[j].gain) * 3;

                //     printf("[!] %d МГц | diff=%d | fpga=%u => score=%d RSSI.symbol = %u, preamble = %u Gain: %d dB\n",
                //         cluster[j].freq_mhz, diff, fpga, score, cluster[j].symbol, cluster[j].preamble, cluster[j].gain);


                //     // printf("[!] Детекція сигналу на частоті %d МГц\n", cluster[j].freq_mhz);
                //     // printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, cluster[j].fpga_value);
                //     // printf("RSSI.symbol = %u, preamble = %u, Gain: %d dB, diff  = %d\n",
                //     //     cluster[j].symbol, cluster[j].preamble, cluster[j].gain, diff);

                //     if (score > best_score) {
                //         best_score = score;
                //         best_freq = cluster[j].freq_mhz;
                //     }
                // }

                for (int j = 0; j < cluster_size; j++) {
                    float norm_symbol = (float)cluster[j].symbol / powf(10.0f, cluster[j].gain / 10.0f);
                    float norm_preamble = (float)cluster[j].preamble / powf(10.0f, cluster[j].gain / 10.0f);
                    float norm_diff = fabsf(norm_symbol - norm_preamble); // нормалізована різниця

                    uint32_t fpga = cluster[j].fpga_value;

                    // Евристика: більше fpga, менший norm_diff
                    int score = (int)fpga + (int)(norm_diff * 100.0f); // масштабуємо, щоб привести до цілих чисел

                    printf("[!] %d МГц | norm_diff=%.2f | fpga=%u => score=%d RSSI.symbol = %u, preamble = %u Gain: %d dB\n",
                        cluster[j].freq_mhz, norm_diff, fpga, score, cluster[j].symbol, cluster[j].preamble, cluster[j].gain);

                    if (score > best_score) {
                        best_score = score;
                        best_freq = cluster[j].freq_mhz;
                    }
                }

                if (best_freq >= 0) {
                    printf(">>> Центральна частота за евристикою: %d МГц (score = %d)\n\n", best_freq, best_score);
                }

                cluster_size = 0;
                collecting = false;
                silent_counter = 0;
            }
        }
        // ad9361_get_rx_rf_gain(phy, 0, &gain);

        // printf("[!] Детекція сигналу на частоті %d МГц\n", freq_mhz);
        // printf("FPGA регістр (0x%lX): %u\n", (unsigned long)reg_addr, value);
        // printf("RSSI.symbol = %u, preamble = %u, mult = %d, dur = %u\n",
        // rssi.symbol, rssi.preamble, rssi.multiplier, rssi.duration);
    }

    // Якщо завершили цикл, але ще був активний кластер — обробити
    if (collecting && cluster_size > 0) {
        int min_diff = abs((int)cluster[0].symbol - (int)cluster[0].preamble);
        int best_freq = cluster[0].freq_mhz;

        for (int j = 1; j < cluster_size; j++) {
            int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
            if (diff > min_diff) {
                min_diff = diff;
                best_freq = cluster[j].freq_mhz;
            }
        }

        printf(">>> Виявлено центральну частоту: %d МГц (max. |symbol - preamble| = %d)\n\n", best_freq, min_diff);
    }

    free(cluster);
    fclose(f);
    circle_time = ((delay * (STOP_FREQ - START_FREQ)) / 1000);
    printf("time spent for one circle is %d ms\n", circle_time);
}


#define default_init_param init_param

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

    // ad9361_set_rx_gain_control_mode(ad9361_phy, 0, RF_GAIN_MGC);
    //  ad9361_set_rx_rf_gain(ad9361_phy, 0, 0);

    ad9361_set_rx_rf_bandwidth(ad9361_phy, 5000000); // 10 МГц
	ad9361_set_rx_sampling_freq(ad9361_phy, 30000000);

    sleep(1);

    struct rf_rssi rssi;
    int32_t gain;

	uint16_t freqs_amount = sizeof(freqs) / sizeof(freqs[0]);
    printf("Кількість частот: %d\n", freqs_amount);

    while (true)
    { 

        if (argc >= 2)
        {
            if (ad9361_get_rx_rssi(ad9361_phy, 0, &rssi) == 0)
                printf("RX0 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
                // printf("RX0 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
            else
                printf("Failed to read RX0 RSSI\n");

            ad9361_get_rx_rf_gain(ad9361_phy, 0, &gain);
            printf("Gain: %d dB\n", gain);
    
            if (ad9361_get_rx_rssi(ad9361_phy, 1, &rssi) == 0)
                printf("RX1 sym = %u, mult = %d", rssi.symbol, rssi.multiplier);
                // printf("RX1 RSSI: %.1f dB\n", (double)rssi.symbol);
                // printf("RX1 RSSI: %.1f dB   ",(double)rssi.symbol * (double)rssi.multiplier / 1000.0);
            else
                printf("Failed to read RX1 RSSI\n");

            ad9361_get_rx_rf_gain(ad9361_phy, 1, &gain);
            printf("Gain: %d dB\n", gain);

            off_t my_register = 0x43C00000;  // заміни на свою адресу
            uint32_t value = fpga_read_reg(my_register);
            //printf("Зчитано з 0x%lX: 0x%08X\n", (unsigned long)my_register, value);
            printf("Зчитано з 0x%lX: %d\n", (unsigned long)my_register, value);

            printf("while\n");
            sleep(1);
        }
        else
        {
            off_t my_register = 0x43C00000;
		    //scan_frequencies(ad9361_phy, freqs, freqs_amount, my_register);
            //scan_frequencies_for_manual_mode(ad9361_phy, freqs, freqs_amount, my_register);
            //scan_frequencies_2(ad9361_phy, freqs, freqs_amount, my_register);
            scan_frequencies_4(ad9361_phy, freqs, freqs_amount, my_register);
        }

    }

    sleep(1);

}