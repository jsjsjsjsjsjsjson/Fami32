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
    DPCM_SAMPLE,
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

const float dpcm_pitch_table[16] {
    4181.71f, 4709.93f, 5264.04f, 5593.04f, 6257.95f, 7046.35f, 7919.35f, 8363.43f, 9419.86f, 11186.08f, 12604.04f, 13982.60f, 16884.65f, 21306.82f, 24857.96f, 33143.94f
};

float phase = 0.0f;
float phase_increment = 0.0f;
uint16_t shift_register = 1;
uint32_t freq_n;

void update_noise_freq(float freq) {
    freq_n = freq;
    phase_increment = freq / SAMP_RATE;
    phase = 0.0f;
}

// int16_t nes_noise_make(bool mode) {
//     int32_t sum_output = 0;

//     for (uint8_t i = 0; i < 4; i++) {
//         phase += phase_increment;
//         if (phase >= 1.0f) {
//             phase -= 1.0f;

//             uint16_t feedback_bit = mode ? 6 : 1;
//             uint16_t feedback = ((shift_register & 1) ^ ((shift_register >> feedback_bit) & 1)) << 14;
//             shift_register = (shift_register >> 1) | feedback;
//         }

//         sum_output += (shift_register & 1) ? -8 : 7;
//     }

//     return sum_output / 4;
// }

int16_t nes_noise_make(bool mode) {
    if (!freq_n) {
        return 0;
    }
    static uint16_t shift_register = 1;
    static uint32_t period_counter = 0;

    float oversampling_factor = freq_n > SAMP_RATE ? freq_n / (float)SAMP_RATE : 1;
    int32_t sum_output = 0;

    for (uint8_t i = 0; i < oversampling_factor; i++) {
        if (period_counter == 0) {
            period_counter = (freq_n > SAMP_RATE ? freq_n : SAMP_RATE) / freq_n;
            uint16_t feedback_bit = mode ? 6 : 1;
            uint16_t feedback = ((shift_register & 1) ^ ((shift_register >> feedback_bit) & 1)) << 14;
            shift_register = (shift_register >> 1) | feedback;
        }
        period_counter--;
        sum_output += (shift_register & 1) ? -8 : 7;
    }
    return sum_output / oversampling_factor;
}

const int8_t vib_table[64] = 
{0, 2, 3, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 15, 16, 16, 16, 16, 16, 15, 15, 14, 13, 12, 11, 10, 9, 8, 6, 5, 3, 2, 0, -2, -3, -5, -6, -8, -9, -10, -11, -12, -13, -14, -15, -15, -16, -16, -16, -16, -16, -15, -15, -14, -13, -12, -11, -10, -9, -8, -6, -5, -3, -2};

#endif