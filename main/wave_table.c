#include "wave_table.h"

static float phase = 0.0f;
static float phase_increment = 0.0f;
static uint32_t freq_n;

void update_noise_freq(float freq) {
    freq_n = freq;
    phase_increment = freq / SAMP_RATE;
    phase = 0.0f;
}

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