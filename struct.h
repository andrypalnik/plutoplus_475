
#include <stdint.h>

typedef struct {
    int freq_mhz;
    uint32_t symbol;
    uint32_t preamble;
    uint32_t fpga_value;
    int32_t gain;
    int16_t low_level;
    int16_t black_level;
} rssi_data_t;