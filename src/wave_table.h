#ifndef WAVE_TABLE_H
#define WAVE_TABLE_H

#include <stdint.h>
#include "src_config.h"

typedef enum {
    PULSE_125,
    PULSE_025,
    PULSE_050,
    PULSE_075,
    TRIANGULAR,
    NOISE0,
    NOISE1,
} WAVE_TYPE;

const int8_t wave_table[5][32] = {
    {-8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7},
    {-8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7, 7, 7, 7, 7},
    {-8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {-8, -8, -8, -8, -8, -8, -8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8}
};

const float noise_freq_table[2][16]
 = {
    {440.0f, 879.9f, 1761.6f, 2348.8f, 3523.2f, 4709.9f, 7046.3f, 8860.3f, 11186.1f, 13982.6f, 18643.5f, 27965.2f, 55930.4f, 111860.8f, 223721.6f, 447443.2f},
    {4.7f, 9.5f, 18.9f, 25.3f, 37.9f, 50.6f, 75.8f, 95.3f, 120.3f, 150.4f, 200.5f, 300.7f, 601.4f, 1202.8f, 2405.6f, 4811.2f}
    // {350, 700, 1330, 1750, 2660, 3570, 5320, 6650, 8400, 10500, 14070, 21070, 42070, 84210, 168420, 336770}
};

float phase = 0.0f;
float phase_increment = 0.0f;
uint16_t shift_register = 1;

void update_noise_freq(float freq) {
    phase_increment = freq / SAMP_RATE;
    phase = 0.0f;
}

int16_t nes_noise_make(bool mode) {
    int32_t sum_output = 0;

    for (uint8_t i = 0; i < 4; i++) {
        phase += phase_increment;
        if (phase >= 1.0f) {
            phase -= 1.0f;

            uint16_t feedback_bit = mode ? 6 : 1;
            uint16_t feedback = ((shift_register & 1) ^ ((shift_register >> feedback_bit) & 1)) << 14;
            shift_register = (shift_register >> 1) | feedback;
        }

        sum_output += (shift_register & 1) ? 15 : -16;
    }

    return sum_output / 4;
}

#endif