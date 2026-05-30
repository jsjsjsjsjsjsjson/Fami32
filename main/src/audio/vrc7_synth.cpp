#include "vrc7_synth.h"

VRC7_SYNTH::VRC7_SYNTH() = default;

VRC7_SYNTH::~VRC7_SYNTH() {
    if (opll != nullptr) {
        OPLL_delete(opll);
        opll = nullptr;
    }
}

int16_t VRC7_SYNTH::clamp_i16(int32_t value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}

void VRC7_SYNTH::init(uint32_t sample_rate) {
    if (opll != nullptr) {
        OPLL_delete(opll);
        opll = nullptr;
    }

    opll = OPLL_new(OPL_CLOCK, sample_rate);
    if (opll == nullptr) {
        ready = false;
        return;
    }

    OPLL_reset(opll);
    OPLL_reset_patch(opll, OPLL_VRC7_TONE);
    last_sample = 0;
    memset(last_channel_sample, 0, sizeof(last_channel_sample));
    memset(channel_sample, 0, sizeof(channel_sample));
    ready = true;
}

void VRC7_SYNTH::reset() {
    if (opll != nullptr) {
        OPLL_reset(opll);
        OPLL_reset_patch(opll, OPLL_VRC7_TONE);
    }
    last_sample = 0;
    memset(last_channel_sample, 0, sizeof(last_channel_sample));
    memset(channel_sample, 0, sizeof(channel_sample));
}

void VRC7_SYNTH::write_reg(uint8_t reg, uint8_t value) {
    if (opll != nullptr) {
        OPLL_writeReg(opll, reg, value);
    }
}

int16_t VRC7_SYNTH::calc() {
    if (opll == nullptr) {
        memset(channel_sample, 0, sizeof(channel_sample));
        return 0;
    }

    int32_t raw = OPLL_calc(opll);
    if (raw > 3600) raw = 3600;
    if (raw < -3200) raw = -3200;

    int32_t sample = (int32_t)((float)raw * AMPLIFY);
    sample = clamp_i16(sample);
    int16_t out = (int16_t)((sample + last_sample) >> 1);
    last_sample = (int16_t)sample;

    /*
     * OPLL_calc() returns the mixed VRC7 output, but the oscilloscope page
     * draws FAMI_CHANNEL::tick_buf for each tracker channel.  emu2413 already
     * exposes the current carrier output per melodic channel via
     * OPLL_getchanvol(); capture those samples here so the player can copy
     * them into the six FM channel buffers without running a second OPLL.
     */
    for (uint8_t ch = 0; ch < CHANNEL_COUNT; ++ch) {
        int32_t ch_raw = OPLL_getchanvol(ch) << 3;
        if (ch_raw > 3600) ch_raw = 3600;
        if (ch_raw < -3200) ch_raw = -3200;

        int32_t ch_scaled = (int32_t)((float)ch_raw * AMPLIFY);
        ch_scaled = clamp_i16(ch_scaled);
        channel_sample[ch] = (int16_t)((ch_scaled + last_channel_sample[ch]) >> 1);
        last_channel_sample[ch] = (int16_t)ch_scaled;
    }

    return out;
}

int16_t VRC7_SYNTH::get_channel_sample(uint8_t channel) const {
    if (channel >= CHANNEL_COUNT) {
        return 0;
    }
    return channel_sample[channel];
}

bool VRC7_SYNTH::is_ready() const {
    return ready;
}
