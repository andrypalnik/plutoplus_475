#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
//#include <bits/mathcalls.h>
#include "linux_spi.h"
#include "linux_gpio.h"
#include "no_os_delay.h" 
#include "no_os_spi.h"
#include "no_os_error.h"
#include "no_os_axi_io.h"
#include "no_os_alloc.h"
//#include "no-OS/drivers/axi_core/axi_dmac/axi_dmac.h"

#include "ad9361.h"
#include "ad9361_api.h"

#define START_FREQ   5500
#define STOP_FREQ    5950  
#define MAX_SILENT_STEPS 3
#define RSSI_OFFSET_DBM -41.7  // Емпірично підібраний offset

uint32_t fpga_read_reg(off_t phys_addr);
void scan_frequencies(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr);
void scan_frequencies_2(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr);
void scan_frequencies_3(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr);
void scan_frequencies_4(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr);
void scan_frequencies_for_manual_mode(struct ad9361_rf_phy *phy, int *freqs, uint16_t count, off_t reg_addr);

int silent_counter = 0;


#if 1
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

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

                //1)-----------------------------------------------------------------------------------------------------------
                // int best_freq = -1;
                // int best_score = -1;

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

                // if (best_freq >= 0) {
                //     printf(">>> Центральна частота за евристикою: %d МГц (score = %d)\n\n", best_freq, best_score);
                // }

                // 2)----------------------------------------------------------------------------------------------------------------------------
                // int best_freq = -1;
                // int best_score = -1;

                // for (int j = 0; j < cluster_size; j++) {
                //     float norm_symbol = (float)cluster[j].symbol / powf(10.0f, cluster[j].gain / 10.0f);
                //     float norm_preamble = (float)cluster[j].preamble / powf(10.0f, cluster[j].gain / 10.0f);
                //     float norm_diff = fabsf(norm_symbol - norm_preamble); // нормалізована різниця

                //     uint32_t fpga = cluster[j].fpga_value;

                //     // Евристика: більше fpga, менший norm_diff
                //     int score = (int)fpga + (int)(norm_diff * 100.0f); // масштабуємо, щоб привести до цілих чисел

                //     printf("[!] %d МГц | norm_diff=%.2f | fpga=%u => score=%d RSSI.symbol = %u, preamble = %u Gain: %d dB\n",
                //         cluster[j].freq_mhz, norm_diff, fpga, score, cluster[j].symbol, cluster[j].preamble, cluster[j].gain);

                //     if (score > best_score) {
                //         best_score = score;
                //         best_freq = cluster[j].freq_mhz;
                //     }
                // }

                // if (best_freq >= 0) {
                //     printf(">>> Центральна частота за евристикою: %d МГц (score = %d)\n\n", best_freq, best_score);
                // }

                // 3 ----------------------------------------------------------------------------------------
                // float max_diff = 1.0f;
                // float max_fpga = 1.0f;

                // // Спочатку знайдемо max для нормалізації
                // for (int j = 0; j < cluster_size; j++) {
                //     int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                //     if (diff > max_diff) max_diff = diff;
                //     if (cluster[j].fpga_value > max_fpga) max_fpga = cluster[j].fpga_value;
                // }

                // int best_freq = -1;
                // float best_score = -1e9;

                // for (int j = 0; j < cluster_size; j++) {
                //     int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                //     uint32_t fpga = cluster[j].fpga_value;
                //     int gain = cluster[j].gain;

                //     float norm_diff = (float)diff / max_diff;
                //     float norm_fpga = (float)fpga / max_fpga;
                //     float norm_symbol = (float)cluster[j].symbol / powf(10.0f, gain / 10.0f);
                //     float norm_preamble = (float)cluster[j].preamble / powf(10.0f, gain / 10.0f);
                //     float norm_symbol_diff = fabsf(norm_symbol - norm_preamble);

                
                //     float score = norm_fpga + norm_diff;

                //     printf("[!] %d МГц | norm_diff=%.2f | norm_fpga=%.2f | norm_sym_diff=%.2f => score=%.2f RSSI.symbol = %u, preamble = %u Gain: %d dB\n",
                //         cluster[j].freq_mhz, norm_diff, norm_fpga, norm_symbol_diff, score,
                //         cluster[j].symbol, cluster[j].preamble, gain);

                //     if (score > best_score) {
                //         best_score = score;
                //         best_freq = cluster[j].freq_mhz;
                //     }
                // }

                // printf(">>> Центральна частота за евристикою: %d МГц (score = %.2f)\n", best_freq, best_score);

                // 4----------------------------------------------------------------------------------------------------------------
                // Знайди мін/макс для нормалізації
                
                uint32_t max_fpga = 1;
                int max_gain = 1, min_gain = 255;
                int max_positive_diff = 1;
                int max_negative_diff = 1;

                for (int j = 0; j < cluster_size; j++) {
                    int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                    if (cluster[j].fpga_value > max_fpga) max_fpga = cluster[j].fpga_value;
                    if (cluster[j].gain > max_gain) max_gain = cluster[j].gain;
                    if (cluster[j].gain < min_gain) min_gain = cluster[j].gain;

                    if (cluster[j].preamble > cluster[j].symbol && diff > max_positive_diff)
                        max_positive_diff = diff;
                    if (cluster[j].preamble < cluster[j].symbol && diff > max_negative_diff)
                        max_negative_diff = diff;
                }

                // Обрахунок score
                float best_score = -1e9;
                int best_freq = -1;

                for (int j = 0; j < cluster_size; j++) {
                    uint32_t fpga = cluster[j].fpga_value;
                    int gain = cluster[j].gain;
                    int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                    int dir = cluster[j].preamble - cluster[j].symbol;

                    float norm_fpga = (float)fpga / max_fpga;
                    //float norm_gain = (float)(max_gain - gain) / (max_gain - min_gain + 1); // +1 to avoid div0

                    float norm_gain = logf(max_gain + 1.0f) - logf(gain + 1.0f);
                          norm_gain /= logf(max_gain + 1.0f) - logf(min_gain + 1.0f);
                    float norm_diff;

                    if (dir > 0) { // preamble > symbol
                        norm_diff = (float)diff / max_positive_diff;
                    } else if (dir < 0) { // preamble < symbol
                        norm_diff = 1.0f - (float)diff / max_negative_diff;
                    } else {
                        norm_diff = 1.0f; // ідеально
                    }

                    // Загальний score
                    //float score = norm_fpga * 1.0f + norm_gain * 1.0f + norm_diff * 1.0f;

                    float score = norm_fpga * 1.0f  + norm_diff * 1.0f;

                    printf("[!] %d МГц | norm_fpga=%.2f norm_gain=%.2f norm_diff=%.2f => score=%.2f | symbol=%u preamble=%u gain=%d fpga=%u\n",
                        cluster[j].freq_mhz, norm_fpga, norm_gain, norm_diff, score,
                        cluster[j].symbol, cluster[j].preamble, gain, fpga);

                    if (score > best_score) {
                        best_score = score;
                        best_freq = cluster[j].freq_mhz;
                    }
                }

                printf(">>> Центральна частота за евристикою: %d МГц (score = %.2f)\n", best_freq, best_score);
                

                /*
                // 6------------------------------------------------------------------------------------------------------------------
                // z scale - works strange not stable
                // 1. Попередній збір статистики
                float sum_fpga = 0, sum_fpga_sq = 0;
                float sum_gain = 0, sum_gain_sq = 0;
                uint32_t max_fpga = 1;
                int min_gain = 255, max_gain = 1;
                int max_positive_diff = 1;
                int max_negative_diff = 1;

                int diffs[cluster_size];
                int dirs[cluster_size];

                for (int j = 0; j < cluster_size; j++) {
                    uint32_t fpga = cluster[j].fpga_value;
                    int gain = cluster[j].gain;
                    int diff = abs((int)cluster[j].symbol - (int)cluster[j].preamble);
                    int dir = (int)cluster[j].preamble - (int)cluster[j].symbol;

                    // Для Z-факторів
                    sum_fpga += fpga;
                    sum_fpga_sq += fpga * fpga;
                    if (fpga > max_fpga) max_fpga = fpga;

                    sum_gain += gain;
                    sum_gain_sq += gain * gain;
                    if (gain < min_gain) min_gain = gain;
                    if (gain > max_gain) max_gain = gain;

                    if (dir > 0 && diff > max_positive_diff) max_positive_diff = diff;
                    if (dir < 0 && diff > max_negative_diff) max_negative_diff = diff;

                    diffs[j] = diff;
                    dirs[j] = dir;
                }

                // Z-нормалізація
                float mean_fpga = sum_fpga / cluster_size;
                float std_fpga = sqrtf(sum_fpga_sq / cluster_size - mean_fpga * mean_fpga + 1e-6f);

                float mean_gain = sum_gain / cluster_size;
                float std_gain = sqrtf(sum_gain_sq / cluster_size - mean_gain * mean_gain + 1e-6f);

                // Пошук кращої частоти
                float best_score = -1e9;
                int best_freq = -1;

                for (int j = 0; j < cluster_size; j++) {
                    uint32_t fpga = cluster[j].fpga_value;
                    int gain = cluster[j].gain;
                    int diff = diffs[j];
                    int dir = dirs[j];

                    // Z-нормалізація
                    float z_fpga = (fpga - mean_fpga) / std_fpga;
                    float z_gain = (gain - mean_gain) / std_gain;
                    z_gain *= -1; // бо менший gain — кращий (сильніший сигнал)

                    // norm_diff по твоїй евристиці
                    float norm_diff = 1.0f;
                    if (dir > 0 && max_positive_diff > 0)
                        norm_diff = (float)diff / max_positive_diff;
                    else if (dir < 0 && max_negative_diff > 0)
                        norm_diff = 1.0f - (float)diff / max_negative_diff;

                    // Підсумковий score
                    float score = z_fpga + z_gain + norm_diff;

                    printf("[!] %d МГц | z_fpga=%.2f z_gain=%.2f norm_diff=%.2f => score=%.2f | symbol=%u preamble=%u gain=%d fpga=%u\n",
                        cluster[j].freq_mhz, z_fpga, z_gain, norm_diff, score,
                        cluster[j].symbol, cluster[j].preamble, gain, fpga);

                    if (score > best_score) {
                        best_score = score;
                        best_freq = cluster[j].freq_mhz;
                    }
                }

                printf(">>> Центральна частота за Z-нормалізацією + евристикою diff: %d МГц (score = %.2f)\n", best_freq, best_score);
                */

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
