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
#include <time.h>
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
//#include "main.h"
#include "struct.h"

// #define START_FREQ   5500
// #define STOP_FREQ    5950  
#define START_FREQ   700
#define STOP_FREQ    5950  
#define MAX_SILENT_STEPS 5
#define RSSI_OFFSET_DBM -41.7  // Емпірично підібраний offset



uint32_t fpga_read_reg(off_t phys_addr);

int scan_frequencies_s(struct ad9361_rf_phy *phy, rssi_data_t *out_array, int max_count, off_t reg_addr);
int scan_frequencies_bl(struct ad9361_rf_phy *phy, rssi_data_t *out_array, int max_count, off_t reg_addr);
int silent_counter = 0;

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

int scan_frequencies_bl(struct ad9361_rf_phy *phy, rssi_data_t *out_array, int max_count, off_t reg_addr)
{
    struct timespec start, end;
    struct rf_rssi rssi;
    uint32_t value;
    uint32_t tmp;
    int16_t low_level;
    int16_t black_level;
    int32_t gain;   
    const int delay = 1500;
    int circle_time;
    int cluster_size = 0;
    bool collecting = false;
    int result_count = 0;  // до циклу

    // clock_gettime(CLOCK_MONOTONIC, &start);
    rssi_data_t *cluster = malloc(sizeof(rssi_data_t) * (STOP_FREQ - START_FREQ + 1));
    if (!cluster) {
        perror("malloc");
        return -1;
    }
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
    // printf("⏱ Block malloc took %ld us\n", delta_us);


    for (uint16_t i = START_FREQ; i <= STOP_FREQ; i += 1) {
        int freq_mhz = i;
        uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;
        
        // clock_gettime(CLOCK_MONOTONIC, &start);
        ad9361_set_rx_lo_freq(phy, freq_hz);

        //          clock_gettime(CLOCK_MONOTONIC, &end);
        //  long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
        //  printf("⏱ Block set_rx_freq took %ld us\n", delta_us);
        usleep(delay);

        value = fpga_read_reg(reg_addr);
        tmp = fpga_read_reg(0x43C00004);

        low_level = (int16_t)(tmp & 0xFFFF);

        tmp = fpga_read_reg(0x43C00008);

        black_level = (int16_t)(tmp & 0xFFFF);

        if (value >10000) {

            collecting = true;
            silent_counter = 0; // обнуляємо мовчання
            // clock_gettime(CLOCK_MONOTONIC, &start);

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

                //printf("%d,%.2f,%d,%u, cluster_size = %d\n", freq_mhz, rssi_dbm, gain, value, cluster_size);

                cluster[cluster_size++] = (rssi_data_t){
                    .freq_mhz = freq_mhz,
                    .symbol = rssi.symbol,
                    .preamble = rssi.preamble,
                    .fpga_value = value,
                    .gain = gain,
                    .low_level = low_level,
                    .black_level = black_level
                };
            }

             //clock_gettime(CLOCK_MONOTONIC, &end);
             //long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
             //printf("⏱ Block rx_rssi took %ld us\n", delta_us);

        } else if (collecting && cluster_size > 0) {

            // clock_gettime(CLOCK_MONOTONIC, &start);
            silent_counter++;

            if (silent_counter >= MAX_SILENT_STEPS && cluster_size > 0) {
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

                int peak_idx = 0;
                uint32_t peak_fpga = cluster[0].fpga_value;
                for (int j = 1; j < cluster_size; j++) {
                    if (cluster[j].fpga_value > peak_fpga) {
                        peak_fpga = cluster[j].fpga_value;
                        peak_idx = j;
                    }
                }

                for (int j = 0; j < cluster_size; j++) {

                    float dist_to_peak = fabsf((float)(j - peak_idx)) / (float)cluster_size;
                    float center_score = 1.0f - dist_to_peak;    

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

                    float score = norm_fpga * 1.0f  + norm_diff * 1.0f;// + center_score * 1.0f;

                    // printf("[!] %d МГц | norm_fpga=%.2f norm_gain=%.2f norm_diff=%.2f dist_to_peak=%.2f center_score=%.2f => score=%.2f | symbol=%u preamble=%u gain=%d fpga=%u\n",
                    //     cluster[j].freq_mhz,
                    //     norm_fpga,
                    //     norm_gain,
                    //     norm_diff,
                    //     dist_to_peak,
                    //     center_score,
                    //     score,
                    //     cluster[j].symbol,
                    //     cluster[j].preamble,
                    //     gain,
                    //     fpga);


                    // if (score > best_score) {
                    //     best_score = score;
                    //     best_freq = cluster[j].freq_mhz;
                    // }

                    if ( -500 > cluster[j].black_level && cluster[j].black_level > -3500 )//&& score > 1.2)
                    {
                        best_freq = cluster[j].freq_mhz;
                        best_score = score;
                        out_array[result_count++] = cluster[j]; 
                        printf(">>> Центральна частота за евристикою: %d МГц (score = %.2f)\n", best_freq, best_score);

                    }

                    // clock_gettime(CLOCK_MONOTONIC, &end);
                    // long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
                    // printf("⏱ Block best_freq took %ld us\n", delta_us);

                    printf("[!] %d МГц | norm_fpga=%.2f norm_diff=%.2f  score=%.2f symbol=%u preamble=%u sync_counter=%u low_level=%d black_level=%d gain=%d\n",
                        cluster[j].freq_mhz,
                        norm_fpga,
                        norm_diff,
                        score,
                        cluster[j].symbol,
                        cluster[j].preamble,
                        fpga,
                        cluster[j].low_level,
                        cluster[j].black_level,
                        cluster[j].gain);


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

    free(cluster);
    //circle_time = ((delay * (STOP_FREQ - START_FREQ)) / 1000);
    //printf("time spent for one circle is %d ms\n", circle_time);
    return result_count;   
}

int scan_frequencies_s(struct ad9361_rf_phy *phy, rssi_data_t *out_array, int max_count, off_t reg_addr)
{
    struct timespec start, end;
    struct rf_rssi rssi;
    uint32_t value;
    uint32_t tmp;
    int16_t low_level;
    int16_t black_level;
    int32_t gain;   
    const int delay = 1500;
    int circle_time;
    int cluster_size = 0;
    bool collecting = false;
    int result_count = 0;  // до циклу

    // clock_gettime(CLOCK_MONOTONIC, &start);
    rssi_data_t *cluster = malloc(sizeof(rssi_data_t) * (STOP_FREQ - START_FREQ + 1));
    if (!cluster) {
        perror("malloc");
        return -1;
    }
    // clock_gettime(CLOCK_MONOTONIC, &end);
    // long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
    // printf("⏱ Block malloc took %ld us\n", delta_us);


    for (uint16_t i = START_FREQ; i <= STOP_FREQ; i += 1) {
        int freq_mhz = i;
        uint64_t freq_hz = (uint64_t)freq_mhz * 1000000ULL;
        
        // clock_gettime(CLOCK_MONOTONIC, &start);
        ad9361_set_rx_lo_freq(phy, freq_hz);

        //          clock_gettime(CLOCK_MONOTONIC, &end);
        //  long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
        //  printf("⏱ Block set_rx_freq took %ld us\n", delta_us);
        usleep(delay);

        value = fpga_read_reg(reg_addr);
        tmp = fpga_read_reg(0x43C00004);

        low_level = (int16_t)(tmp & 0xFFFF);

        tmp = fpga_read_reg(0x43C00008);

        black_level = (int16_t)(tmp & 0xFFFF);

        if (value >10000) {

            collecting = true;
            silent_counter = 0; // обнуляємо мовчання
            // clock_gettime(CLOCK_MONOTONIC, &start);

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

                //printf("%d,%.2f,%d,%u, cluster_size = %d\n", freq_mhz, rssi_dbm, gain, value, cluster_size);

                cluster[cluster_size++] = (rssi_data_t){
                    .freq_mhz = freq_mhz,
                    .symbol = rssi.symbol,
                    .preamble = rssi.preamble,
                    .fpga_value = value,
                    .gain = gain,
                    .low_level = low_level,
                    .black_level = black_level
                };
            }

             //clock_gettime(CLOCK_MONOTONIC, &end);
             //long delta_us = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_nsec - start.tv_nsec) / 1000;
             //printf("⏱ Block rx_rssi took %ld us\n", delta_us);

        } else if (collecting && cluster_size > 0) {

            // clock_gettime(CLOCK_MONOTONIC, &start);
            silent_counter++;

            if (silent_counter >= MAX_SILENT_STEPS && cluster_size > 0) {
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

                int peak_idx = 0;
                uint32_t peak_fpga = cluster[0].fpga_value;
                for (int j = 1; j < cluster_size; j++) {
                    if (cluster[j].fpga_value > peak_fpga) {
                        peak_fpga = cluster[j].fpga_value;
                        peak_idx = j;
                    }
                }

                for (int j = 0; j < cluster_size; j++) {

                    float dist_to_peak = fabsf((float)(j - peak_idx)) / (float)cluster_size;
                    float center_score = 1.0f - dist_to_peak;    

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

                    float score = norm_fpga * 1.0f  + norm_diff * 1.0f;// + center_score * 1.0f;

                    printf("[!] %d МГц | norm_fpga=%.2f norm_gain=%.2f norm_diff=%.2f dist_to_peak=%.2f center_score=%.2f => score=%.2f | symbol=%u preamble=%u gain=%d fpga=%u\n",
                        cluster[j].freq_mhz,
                        norm_fpga,
                        norm_gain,
                        norm_diff,
                        dist_to_peak,
                        center_score,
                        score,
                        cluster[j].symbol,
                        cluster[j].preamble,
                        gain,
                        fpga);


                    if (score > best_score) {
                        best_score = score;
                        best_freq = cluster[j].freq_mhz;
                    }

                    // if ( -500 > cluster[j].black_level && cluster[j].black_level > -3500 && score > 1.2)
                    // {
                    //     best_freq = cluster[j].freq_mhz;
                    //     best_score = score;
                    //     out_array[result_count++] = cluster[j]; 
                    //     printf(">>> Центральна частота за евристикою: %d МГц (score = %.2f)\n", best_freq, best_score);

                    // }



                    // printf("[!] %d МГц | norm_fpga=%.2f norm_diff=%.2f  score=%.2f symbol=%u preamble=%u sync_counter=%u low_level=%d black_level=%d gain=%d\n",
                    //     cluster[j].freq_mhz,
                    //     norm_fpga,
                    //     norm_diff,
                    //     score,
                    //     cluster[j].symbol,
                    //     cluster[j].preamble,
                    //     fpga,
                    //     cluster[j].low_level,
                    //     cluster[j].black_level,
                    //     cluster[j].gain);


                }
            printf(">>> Центральна частота за евристикою: %d МГц (score = %.2f). ⏱ Block best_freq took %ld us\n", best_freq, best_score);
            clock_gettime(CLOCK_MONOTONIC, &end);


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

    free(cluster);
    //circle_time = ((delay * (STOP_FREQ - START_FREQ)) / 1000);
    //printf("time spent for one circle is %d ms\n", circle_time);
    return result_count;   
}



