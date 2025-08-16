#include "wave_table.h"
#include "note2freq.h"

static const uint16_t noise_periods[16] = {
    4068, 2034, 1016, 762, 508, 380, 254, 202, 160, 128, 96, 64, 32, 16, 8, 4
};

static struct {
    uint16_t shift_reg;
    float timer;
    float period;
} noise = {1, 0.0f, 4.0f};

void nes_noise_set_period(uint8_t index) {
    if (index > 15) index = 15;
    noise.period = (float)noise_periods[index] * SAMP_RATE / FCPU_HZ;
}

int16_t nes_noise_get_sample(bool short_mode) {
    noise.timer += 1.0f;

    int32_t sum = 0;
    int updates = 0;
    
    while (noise.timer >= noise.period) {
        noise.timer -= noise.period;
        
        uint16_t feedback_bit = short_mode ? 6 : 1;
        uint16_t feedback = ((noise.shift_reg & 1) ^ 
                           ((noise.shift_reg >> feedback_bit) & 1)) << 14;
        noise.shift_reg = (noise.shift_reg >> 1) | feedback;

        sum += (noise.shift_reg & 1) ? -8 : 7;
        updates++;

        if (updates > 100) break;
    }

    if (updates > 0) {
        return sum / updates;
    } else {
        return (noise.shift_reg & 1) ? -8 : 7;
    }
}