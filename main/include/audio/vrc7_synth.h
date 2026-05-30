#ifndef VRC7_SYNTH_H
#define VRC7_SYNTH_H

#include <stdint.h>
#include <string.h>

extern "C" {
#include "emu2413.h"
}

class VRC7_SYNTH {
private:
    static constexpr uint32_t OPL_CLOCK = 3579545;
    static constexpr float AMPLIFY = 4.6f;

    static constexpr uint8_t CHANNEL_COUNT = 6;

    OPLL *opll = nullptr;
    int16_t last_sample = 0;
    int16_t last_channel_sample[CHANNEL_COUNT] = {};
    int16_t channel_sample[CHANNEL_COUNT] = {};
    bool ready = false;

    static int16_t clamp_i16(int32_t value);

public:
    VRC7_SYNTH();
    ~VRC7_SYNTH();

    void init(uint32_t sample_rate);
    void reset();
    void write_reg(uint8_t reg, uint8_t value);
    int16_t calc();
    int16_t get_channel_sample(uint8_t channel) const;
    bool is_ready() const;
};

#endif // VRC7_SYNTH_H
