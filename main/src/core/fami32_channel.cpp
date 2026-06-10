#include "fami32_channel.h"

namespace {

int16_t clamp_i16(int32_t value) {
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (int16_t)value;
}

uint8_t clamp_apu_level(int32_t value, uint8_t max_value) {
    if (value < 0) return 0;
    if (value > max_value) return max_value;
    return (uint8_t)value;
}

bool is_noise_mode(WAVE_TYPE mode) {
    return mode == NOISE0 || mode == NOISE1;
}

bool is_tone_mode(WAVE_TYPE mode) {
    return mode < DPCM_SAMPLE;
}

bool is_vrc6_pulse_mode(WAVE_TYPE mode) {
    return mode >= VRC6_PULSE_062 && mode <= VRC6_PULSE_500;
}

bool is_vrc6_mode(WAVE_TYPE mode) {
    return is_vrc6_pulse_mode(mode) || mode == VRC6_SAW;
}

const uint16_t VRC7_FREQ_TABLE[12] = {
    172, 183, 194, 205, 217, 230, 244, 258, 274, 290, 307, 326
};

void note_to_vrc7_pitch(uint8_t fami32_note, uint16_t &fnum, uint8_t &block) {
    /*
     * Fami32 stores tracker notes as NES/MIDI-style notes where tracker C-0 is
     * represented as MIDI note 24.  FamiTracker's VRC7 handler uses C-0 as
     * note index 0 for the OPLL block/f-number table.
     */
    uint8_t opll_note = (fami32_note >= 24) ? (fami32_note - 24) : fami32_note;
    fnum = VRC7_FREQ_TABLE[opll_note % 12];
    block = opll_note / 12;
    if (block > 7) {
        block = 7;
    }
}

void period_to_vrc7_pitch(float period, uint16_t &fnum, uint8_t &block) {
    if (period < 0.0f) {
        period = 0.0f;
    }
    uint8_t rounded_note = (uint8_t)period2note(period);
    note_to_vrc7_pitch(rounded_note, fnum, block);
}

const uint8_t VRC7_DEFAULT_REGS[8] = {0x01, 0x21, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F};
const int8_t FDS_MOD_BIAS[8] = {0, 1, 2, 4, 0, -4, -2, -1};
const uint16_t NOISE_PERIODS[16] = {
    4068, 2034, 1016, 762, 508, 380, 254, 202,
    160, 128, 96, 64, 32, 16, 8, 4
};
constexpr int NOISE_OVERSAMPLE_MAX = 64;
constexpr float PHASE_SCALE = 4294967296.0f;
constexpr int FIR_HALF_TAPS = FIR_TAPS / 2;
constexpr int16_t FIR_COEFS[FIR_HALF_TAPS + 1] = {
    14, 242, 1021, 1542
};
static_assert((FIR_TAPS & 1) == 1, "FIR_TAPS must be odd");

uint32_t phase_step_from_cycles(float cycles_per_sample) {
    if (cycles_per_sample <= 0.0f) {
        return 0;
    }
    if (cycles_per_sample >= 1.0f) {
        return UINT32_MAX;
    }
    return (uint32_t)(cycles_per_sample * PHASE_SCALE + 0.5f);
}

uint8_t wave_phase_index(uint32_t phase) {
    return (uint8_t)(phase >> 27);
}

uint8_t fds_phase_index(uint32_t phase) {
    return (uint8_t)(phase >> 26);
}

float midi_note_to_freq(uint8_t midi_note) {
    return BASE_FREQ_HZ * exp2f(((float)midi_note - 69.0f) / 12.0f);
}

float note_to_fds_period(uint8_t midi_note) {
    // FDS uses a direct 12-bit frequency register, not the inverted 2A03
    // timer period.  This matches FamiTracker's FDS lookup table:
    //   reg = freq * 65536 / ((CPU / 16) / 4)
    float reg = (midi_note_to_freq(midi_note) * 16384.0f) / (FCPU_HZ / 64.0f) + 0.5f;
    if (reg < 0.0f) reg = 0.0f;
    if (reg > 4095.0f) reg = 4095.0f;
    return reg;
}

float fds_period_to_freq(float period) {
    if (period < 0.0f) period = 0.0f;
    if (period > 4095.0f) period = 4095.0f;
    return period * (FCPU_HZ / 64.0f) / 65536.0f;
}

uint8_t current_n163_channels(FTM_FILE *data) {
    if (data == NULL || !data->n163_enabled()) {
        return 1;
    }
    uint8_t channels = data->n163_channel_count();
    if (channels < 1) return 1;
    if (channels > FAMI32_N163_MAX_CHANNELS) return FAMI32_N163_MAX_CHANNELS;
    return channels;
}

float n163_freq_to_period(float freq, uint8_t channels) {
    float period = (freq * (float)channels * 983040.0f) / (FCPU_HZ / 16.0f) / 4.0f;
    if (period < 15.0f) period = 15.0f;
    if (period > 65535.0f) period = 65535.0f;
    return period;
}

float note_to_n163_period(uint8_t midi_note, uint8_t channels) {
    return n163_freq_to_period(midi_note_to_freq(midi_note), channels);
}

float n163_period_to_freq(float n163_period, uint8_t channels) {
    if (n163_period < 0.0f) n163_period = 0.0f;
    if (channels < 1) channels = 1;
    return (n163_period * 4.0f) * (FCPU_HZ / 16.0f) / ((float)channels * 983040.0f * 2);
}

float n163_period_to_common_period(float n163_period, uint8_t channels) {
    float freq = n163_period_to_freq(n163_period, channels);
    if (freq <= 0.0f) {
        return 2048.0f;
    }
    return freq2period(freq);
}

float note_to_channel_period(WAVE_TYPE mode, uint8_t midi_note, uint8_t n163_channels = 1) {
    if (mode == FDS_WAVE) {
        return note_to_fds_period(midi_note);
    }
    if (mode == N163_WAVE) {
        return note_to_n163_period(midi_note, n163_channels);
    }
    return note2period(midi_note);
}

uint8_t clamp_midi_note(int note) {
    if (note < 0) return 0;
    if (note > 127) return 127;
    return (uint8_t)note;
}

uint8_t fixed_arp_to_midi_note(int note) {
    if (note < 0) note = 0;
    if (note > 95) note = 95;
    return (uint8_t)(note + 24);
}

uint16_t n163_pitch_effect_speed(WAVE_TYPE mode, uint16_t speed) {
    return mode == N163_WAVE ? (uint16_t)(speed << 2) : speed;
}

bool is_direct_frequency_mode(WAVE_TYPE mode) {
    return mode == FDS_WAVE || mode == N163_WAVE;
}

} // namespace

void FAMI_CHANNEL::clear_all_fx_flag() {
    chl_vol = 15;
    rel_vol = 0;

    freq = 0;
    period = 0;
    phase_acc = 0;

    if (mode == VRC7_FM || mode == FDS_WAVE || mode == N163_WAVE || is_vrc6_mode(mode)) {
        chl_mode = mode;
    } else if (mode < TRIANGULAR) {
        mode = PULSE_125;
        chl_mode = mode;
    } else if (is_noise_mode(mode)) {
        mode = NOISE0;
        chl_mode = mode;
    }

    slide_up = 0;
    slide_down = 0;

    vol_slide_up = 0;
    vol_slide_down = 0;

    vol_count = 0;

    auto_port = 0;
    auto_port_source = 0.0f;
    auto_port_target = 0.0f;
    auto_port_finish = true;

    vib_spd = 0;
    vib_dep = 0;

    vib_pos = 0;

    tre_spd = 0;
    tre_dep = 0;

    tre_pos = 0;

    portup_target = 0.0f;
    portup_speed = 0;

    portdown_target = 0.0f;
    portdown_speed = 0;

    period_offset = 0;

    delay_cut_count = 0;
    delay_cut_status = false;

    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    arp_fx_pos = 0;
    arp_relative_note = 0;
    arp_fixed_active = false;

    sample_pos = 0;
    sample_phase = 0;
    sample_var = 0;
    dmc_hold_level = 0;
    sample_num = 0;
    sample_start = 0;
    sample_status = 0;
    sample_len = 0;
    sample_pitch = 0;

    if (mode == TRIANGULAR) {
        triangle_hold_level = 8;
    }

    if (mode == VRC7_FM) {
        vrc7_gate = false;
        vrc7_release = false;
        vrc7_trigger = true;
        vrc7_fnum = 0;
        vrc7_block = 0;
    }

    if (mode == FDS_WAVE) {
        fds_gate = false;
        fds_phase = 0;
        fds_mod_counter = 0;
    }

    if (mode == N163_WAVE) {
        n163_gate = false;
        n163_phase = 0;
        n163_wave_index = 0;
    }

    last_note = 255;
}

void FAMI_CHANNEL::init(FTM_FILE* data) {
    tick_length = SAMP_RATE / ENG_SPEED;
    nominal_tick_length = tick_length;
    ftm_data = data;
    mode = PULSE_125;
    chl_mode = PULSE_125;
    clear_all_fx_flag();
    inst_proc.init(data);
    tick_buf.resize(tick_length);
    apu_level_buf.resize(tick_length);
    reset_fir_state();
    reset_noise_state();
    DBG_PRINTF("INIT: ftm_data = %p, tick_length = (%d / %d) = %d\n", ftm_data, SAMP_RATE, ENG_SPEED, tick_length);
    hpf.setCutoffFrequency(HPF_CUTOFF, SAMP_RATE);
}

void FAMI_CHANNEL::reset_fir_state() {
    memset(&pcm_fir_state, 0, sizeof(pcm_fir_state));
    memset(&noise_level_fir_state, 0, sizeof(noise_level_fir_state));
}

void FAMI_CHANNEL::reset_noise_state() {
    noise_shift_reg = 1;
    noise_phase = 0;
    update_noise_period();
}

void FAMI_CHANNEL::fir_push(FirState &state, int32_t x) {
    state.hist[state.wr] = x;
    state.wr++;
    if (state.wr >= FIR_TAPS) state.wr = 0;
}

int32_t FAMI_CHANNEL::fir_apply(const FirState &state) const {
    uint8_t center = state.wr + FIR_HALF_TAPS;
    if (center >= FIR_TAPS) center -= FIR_TAPS;

    int32_t acc = state.hist[center] * FIR_COEFS[FIR_HALF_TAPS];

    uint8_t left = state.wr;
    uint8_t right = (state.wr == 0) ? (FIR_TAPS - 1) : (state.wr - 1);

    for (int i = 0; i < FIR_HALF_TAPS; i++) {
        acc += (state.hist[left] + state.hist[right]) * FIR_COEFS[i];
        left++;
        if (left >= FIR_TAPS) left = 0;
        right = (right == 0) ? (FIR_TAPS - 1) : (right - 1);
    }

    constexpr int32_t half = 1 << (FIR_COEF_SHIFT - 1);
    if (acc >= 0) {
        return (acc + half) >> FIR_COEF_SHIFT;
    }
    return -((-acc + half) >> FIR_COEF_SHIFT);
}

void FAMI_CHANNEL::update_noise_period() {
    uint8_t index = noise_rate_rel;
    if (index > 15) index = 15;
    noise_period = (float)NOISE_PERIODS[index] * SAMP_RATE / FCPU_HZ;
    if (noise_period <= 0.0f) {
        noise_period = 1.0f;
    }
}

bool FAMI_CHANNEL::next_noise_bit(bool short_mode, uint32_t phase_step) {
    uint8_t feedback_bit = short_mode ? 6 : 1;
    uint32_t old_phase = noise_phase;
    noise_phase += phase_step;

    if (noise_phase < old_phase) {
        uint16_t feedback = ((noise_shift_reg & 1) ^ ((noise_shift_reg >> feedback_bit) & 1)) << 14;
        noise_shift_reg = (noise_shift_reg >> 1) | feedback;
    }

    return (noise_shift_reg & 1) != 0;
}

void FAMI_CHANNEL::set_slide_up(uint8_t n) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_up = n163_pitch_effect_speed(mode, n);
    slide_down = 0;
    auto_port = 0;
}

void FAMI_CHANNEL::set_slide_down(uint8_t n) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_down = n163_pitch_effect_speed(mode, n);
    slide_up = 0;
    auto_port = 0;
}

void FAMI_CHANNEL::set_vol_slide_up(uint8_t n) {
    vol_count = 0;
    if (n)
        vol_slide_up = roundf(8.0f / n);
    else
        vol_slide_up = 0;
}

void FAMI_CHANNEL::set_vol_slide_down(uint8_t n) {
    vol_count = 0;
    if (n)
        vol_slide_down = roundf(8.0f / n);
    else
        vol_slide_down = 0;
}

void FAMI_CHANNEL::set_auto_port(uint8_t n) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    auto_port = n163_pitch_effect_speed(mode, n);
    if (last_note != 255) {
        auto_port_source = get_period();
        auto_port_finish = true;
    }
    slide_up = 0;
    slide_down = 0;
}

void FAMI_CHANNEL::set_vibrato(uint8_t spd, uint8_t dep) {
    vib_spd = spd;
    vib_dep = dep;

    vib_pos = 0;
}

void FAMI_CHANNEL::set_tremolo(uint8_t spd, uint8_t dep) {
    tre_spd = spd;
    tre_dep = dep;

    tre_pos = 0;
}

void FAMI_CHANNEL::set_period_offset(int8_t var) {
    period_offset = var;
}

void FAMI_CHANNEL::set_port_up(uint8_t spd, uint8_t offset) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_up = 0;
    slide_down = 0;

    portdown_speed = 0;
    auto_port = 0;
    portup_speed = n163_pitch_effect_speed(mode, (spd * 2) + 1);
    portup_target = note_to_channel_period(mode, base_note + offset, current_n163_channels(ftm_data));
    base_note += offset;
}

void FAMI_CHANNEL::set_port_down(uint8_t spd, uint8_t offset) {
    arp_fx_n1 = 0;
    arp_fx_n2 = 0;

    slide_up = 0;
    slide_down = 0;

    portup_speed = 0;
    auto_port = 0;
    portdown_speed = n163_pitch_effect_speed(mode, (spd * 2) + 1);
    portdown_target = note_to_channel_period(mode, base_note - offset, current_n163_channels(ftm_data));
    base_note -= offset;
}

void FAMI_CHANNEL::set_delay_cut(uint8_t t) {
    delay_cut_count = t;
    delay_cut_status = true;
}

void FAMI_CHANNEL::make_tick_sound() {
    make_tick_sound(true);
}

void FAMI_CHANNEL::make_tick_sound(bool advance_tick_phase) {
    if (apu_level_buf.size() != tick_buf.size()) {
        apu_level_buf.resize(tick_buf.size());
    }

    int8_t env_vol = inst_proc.get_sequ_var(VOL_SEQU);
    int oversample = OVER_SAMPLE;
    if (oversample < 1) oversample = 1;

    int8_t tre_var = 0;
    if (tre_spd && tre_dep) {
        if (advance_tick_phase) {
            tre_pos = (tre_pos + tre_spd) & 63;
        }
        tre_var = (vib_table[tre_pos] * tre_dep) / 32;
    }

    if (mode == VRC7_FM) {
        if (env_vol <= 0) {
            env_vol = 15;
        }
        int32_t v = env_vol * (chl_vol + tre_var);
        if (v < 0) v = 0;
        if (v > 225) v = 225;
        rel_vol = (uint8_t)v;
        memset(tick_buf.data(), 0, tick_buf.size() * sizeof(int16_t));
        uint8_t level = (rel_vol + 7) / 15;
        if (level > 15) level = 15;
        memset(apu_level_buf.data(), level, apu_level_buf.size() * sizeof(uint8_t));
        return;
    }

    if (mode == FDS_WAVE) {
        if (env_vol < 0) env_vol = 0;
        if (env_vol > 31) env_vol = 31;
        int32_t fds_volume = (int32_t)env_vol * (chl_vol + tre_var);
        if (fds_volume < 0) fds_volume = 0;
        fds_volume = (fds_volume + 7) / 15;
        if (fds_volume > 31) fds_volume = 31;
        rel_vol = (uint8_t)((fds_volume * 225) / 31);

        period_rely = get_period();
        if (vib_spd && vib_dep) {
            if (advance_tick_phase) {
                vib_pos = (vib_pos + vib_spd) & 63;
            }
            int8_t vib_var = (vib_table[vib_pos] * vib_dep) / 16;
            period_rely = get_period() + vib_var - period_offset;
        }
        if (period_rely < 0.0f) period_rely = 0.0f;
        if (period_rely > 4095.0f) period_rely = 4095.0f;
        freq = fds_period_to_freq(period_rely);

        for (int i = 0; i < tick_length; i++) {
            float mod_scale = 1.0f;
            if (fds_mod_depth && fds_mod_speed && fds_mod_counter >= fds_mod_delay) {
                uint8_t mod_index = (fds_mod_counter >> 12) & (FAMI32_FDS_MOD_SIZE - 1);
                uint8_t mod_value = fds_mod[mod_index] & 7;
                mod_scale += (float)(FDS_MOD_BIAS[mod_value] * (int32_t)(fds_mod_depth & 0x3F)) / 32768.0f;
            }
            uint32_t fds_phase_step = phase_step_from_cycles((freq * mod_scale) / SAMP_RATE);

            int32_t sample = 0;
            if (fds_gate && fds_volume) {
                uint8_t wave_index = fds_phase_index(fds_phase) & (FAMI32_FDS_WAVE_SIZE - 1);
                int32_t centered = (int32_t)(fds_wave[wave_index] & 0x3F) - 32;
                // Match FDS mixing closer to FamiTracker: the previous
                // lightweight path was about 6 dB too quiet.
                sample = (centered * (int32_t)rel_vol) >> 1;
                fds_phase += fds_phase_step;
                fds_mod_counter += fds_mod_speed ? fds_mod_speed : 1;
            }
            tick_buf[i] = clamp_i16(sample);
            apu_level_buf[i] = 0;
        }
        return;
    }

    if (mode == N163_WAVE) {
        if (env_vol < 0) env_vol = 0;
        if (env_vol > 15) env_vol = 15;
        int32_t channel_volume = chl_vol + tre_var;
        if (channel_volume < 0) channel_volume = 0;
        if (channel_volume > 15) channel_volume = 15;
        rel_vol = (uint8_t)(env_vol * channel_volume);

        period_rely = period + ((float)period_offset * 16.0f);
        if (vib_spd && vib_dep) {
            if (advance_tick_phase) {
                vib_pos = (vib_pos + vib_spd) & 63;
            }
            int8_t vib_var = (vib_table[vib_pos] * vib_dep) / 16;
            period_rely = period + (((float)period_offset - (float)vib_var) * 16.0f);
        }
        if (period_rely < 15.0f) period_rely = 15.0f;
        if (period_rely > 65535.0f) period_rely = 65535.0f;
        freq = n163_period_to_freq(period_rely, current_n163_channels(ftm_data));
        uint32_t phase_step = phase_step_from_cycles(freq / ((float)SAMP_RATE * oversample));
        uint8_t meter = (rel_vol + 7) / 15;
        if (meter > 15) meter = 15;
        uint8_t wave_size = n163_wave_size;
        if (wave_size < 1 || wave_size > FAMI32_N163_WAVE_SIZE) {
            wave_size = FAMI32_N163_WAVE_SIZE;
        }

        for (int i = 0; i < tick_length; i++) {
            int32_t pcm_out = 0;
            int32_t level_acc = 0;
            for (int j = 0; j < oversample; j++) {
                int32_t pcm_sub = 0;
                uint8_t level_sub = 0;
                if (n163_gate && rel_vol) {
                    uint8_t wave_index = (uint8_t)(((uint64_t)n163_phase * wave_size) >> 32);
                    int32_t centered = (int32_t)(n163_wave[wave_index] & 0x0F) - 8;
                    pcm_sub = centered * (int32_t)rel_vol;
                    level_sub = centered >= 0 ? meter : 0;
                    n163_phase += phase_step;
                }
                level_acc += level_sub;
                fir_push(pcm_fir_state, pcm_sub);
                if (j == oversample - 1) {
                    pcm_out = fir_apply(pcm_fir_state);
                }
            }
            tick_buf[i] = clamp_i16(pcm_out);
            apu_level_buf[i] = clamp_apu_level((level_acc + (oversample / 2)) / oversample, 15);
        }
        return;
    }

    if (is_vrc6_mode(mode)) {
        if (env_vol < 0) env_vol = 0;
        if (env_vol > 15) env_vol = 15;
        int32_t channel_volume = chl_vol + tre_var;
        if (channel_volume < 0) channel_volume = 0;
        if (channel_volume > 15) channel_volume = 15;

        int32_t base_volume = env_vol * channel_volume;
        rel_vol = (uint8_t)base_volume;

        period_rely = period - period_offset;
        if (vib_spd && vib_dep) {
            if (advance_tick_phase) {
                vib_pos = (vib_pos + vib_spd) & 63;
            }
            int8_t vib_var = (vib_table[vib_pos] * vib_dep) / 16;
            period_rely = period - period_offset + vib_var;
        }
        if (period_rely < 12.0f) period_rely = 12.0f;
        if (period_rely > 4095.0f) period_rely = 4095.0f;
        freq = period2freq(period_rely);
        uint32_t phase_step = phase_step_from_cycles(freq / ((float)SAMP_RATE * oversample));
        uint8_t vrc6_meter = (rel_vol + 7) / 15;
        if (vrc6_meter > 15) vrc6_meter = 15;

        for (int i = 0; i < tick_length; i++) {
            int32_t pcm_out = 0;
            int32_t apu_acc = 0;
            for (int j = 0; j < oversample; j++) {
                int32_t pcm_sub = 0;
                uint8_t apu_sub = 0;
                if (rel_vol) {
                    uint8_t wave_index = wave_phase_index(phase_acc);
                    int8_t wave = wave_table[mode][wave_index];
                    pcm_sub = wave * rel_vol;
                    apu_sub = (wave >= 0) ? vrc6_meter : 0;
                    phase_acc += phase_step;
                }
                apu_acc += apu_sub;
                fir_push(pcm_fir_state, pcm_sub);
                if (j == oversample - 1) {
                    pcm_out = fir_apply(pcm_fir_state);
                }
            }
            tick_buf[i] = clamp_i16(pcm_out);
            apu_level_buf[i] = clamp_apu_level(
                (apu_acc + (oversample / 2)) / oversample,
                15
            );
        }
        return;
    }

    if (env_vol < 0) env_vol = 0;
    if (env_vol > 15) env_vol = 15;
    int32_t channel_volume = chl_vol + tre_var;
    if (channel_volume < 0) channel_volume = 0;
    if (channel_volume > 15) channel_volume = 15;
    rel_vol = (uint8_t)(env_vol * channel_volume);

    if (is_tone_mode(mode)) {
        period_rely = period - period_offset;
        freq = period2freq(period_rely);

        if (vib_spd && vib_dep) {
            if (advance_tick_phase) {
                vib_pos = (vib_pos + vib_spd) & 63;
            }
            int8_t vib_var = (vib_table[vib_pos] * vib_dep) / 16;
            period_rely = period - period_offset + vib_var;
            freq = period2freq(period_rely);
        }

        if (mode == TRIANGULAR) {
            rel_vol = rel_vol ? 225 : 0;
        }
        // DBG_PRINTF("(DEBUG: VOL) GET_ENV: %d->%d\n", inst_proc.get_pos(VOL_SEQU), inst_proc.get_sequ_var(VOL_SEQU));
        float cycles_per_subsample = freq / ((float)SAMP_RATE * oversample);
        if (mode == TRIANGULAR) cycles_per_subsample *= 0.5f;
        uint32_t phase_step = phase_step_from_cycles(cycles_per_subsample);
        uint8_t nes_vol = (rel_vol + 7) / 15;
        if (nes_vol > 15) nes_vol = 15;

        for (int i = 0; i < tick_length; i++) {
            uint8_t apu_max = 15;
            int32_t apu_acc = 0;

            int32_t pcm_out = 0;

            for (int j = 0; j < oversample; j++) {
                int32_t pcm_sub = 0;
                uint8_t apu_sub = (mode == TRIANGULAR) ? triangle_hold_level : 0;

                if (rel_vol) {
                    uint8_t wave_index = wave_phase_index(phase_acc);
                    int8_t wave = wave_table[mode][wave_index];

                    pcm_sub = wave * rel_vol;

                    if (mode == TRIANGULAR) {
                        apu_sub = wave + 8;
                        triangle_hold_level = apu_sub;
                    } else {
                        apu_sub = (wave >= 0) ? nes_vol : 0;
                    }

                    phase_acc += phase_step;
                }

                fir_push(pcm_fir_state, pcm_sub);
                if (j == oversample - 1) {
                    pcm_out = fir_apply(pcm_fir_state);
                }

                apu_acc += apu_sub;
            }

            tick_buf[i] = clamp_i16(pcm_out);
            apu_level_buf[i] = clamp_apu_level(
                (apu_acc + (oversample / 2)) / oversample,
                apu_max
            );
        }
    } else if (is_noise_mode(mode)) {
        uint8_t nes_vol = (rel_vol + 7) / 15;
        if (nes_vol > 15) nes_vol = 15;
        int noise_oversample = OVER_SAMPLE;
        if (noise_oversample < 1) noise_oversample = 1;
        while (noise_oversample < NOISE_OVERSAMPLE_MAX &&
               noise_period * (float)noise_oversample < 1.0f) {
            noise_oversample++;
        }
        uint32_t noise_phase_step = phase_step_from_cycles(1.0f / (noise_period * noise_oversample));
        bool short_mode = mode == NOISE1;

        for (int i = 0; i < tick_length; i++) {
            int32_t pcm_out = 0;
            int32_t level_out = 0;

            for (int j = 0; j < noise_oversample; j++) {
                bool noise_bit = next_noise_bit(short_mode, noise_phase_step);
                int32_t pcm_sub = (((noise_bit ? -128 : 127) * (int32_t)rel_vol) >> 4);
                int32_t level_sub = noise_bit ? 0 : nes_vol;

                fir_push(pcm_fir_state, pcm_sub);
                fir_push(noise_level_fir_state, level_sub);

                if (j == noise_oversample - 1) {
                    pcm_out = fir_apply(pcm_fir_state);
                    level_out = fir_apply(noise_level_fir_state);
                }
            }

            tick_buf[i] = clamp_i16(pcm_out);
            apu_level_buf[i] = clamp_apu_level(level_out, 15);
        }
    } else if (mode == DPCM_SAMPLE) {
        uint32_t sample_phase_step = phase_step_from_cycles((float)dpcm_pitch_table[sample_pitch] / SAMP_RATE);
        if (sample_num >= 0 && sample_num < (int)ftm_data->dpcm_samples.size()) {
            const std::vector<uint8_t> &pcm = ftm_data->dpcm_samples[sample_num].pcm_data;
            uint8_t *sample = ftm_data->dpcm_samples[sample_num].pcm_data.data();
            if (sample_len > pcm.size()) {
                sample_len = pcm.size();
            }
            for (int i = 0; i < tick_length; i++) {
                if (sample_status) {
                    if (sample_pos < 0 || sample_pos >= sample_len) {
                        if (sample_loop && sample_len > 0) {
                            sample_pos = 0;
                        } else {
                            sample_status = false;
                            sample_var = dmc_hold_level;
                            tick_buf[i] = 0;
                            apu_level_buf[i] = dmc_hold_level;
                            continue;
                        }
                    }
                    sample_var = sample[sample_pos];
                    dmc_hold_level = sample_var & 0x7f;
                    uint32_t old_phase = sample_phase;
                    sample_phase += sample_phase_step;
                    if (sample_phase < old_phase) {
                        sample_pos++;
                        if (sample_pos >= sample_len) {
                            if (sample_loop) {
                                sample_pos = 0;
                            } else {
                                sample_status = false;
                            }
                        }
                    }
                }
                // printf("SAMPLE_VAR: %d\n", sample_var);
                tick_buf[i] = hpf.process(sample_var * 64);
                apu_level_buf[i] = dmc_hold_level;
            }
        } else {
            for (int i = 0; i < tick_length; i++) {
                tick_buf[i] = 0;
                apu_level_buf[i] = dmc_hold_level;
            }
        }
    } else {
        memset(tick_buf.data(), 0, tick_buf.size() * sizeof(int16_t));
        memset(apu_level_buf.data(), 0, apu_level_buf.size() * sizeof(uint8_t));
    }
}

void FAMI_CHANNEL::begin_tick_update() {
    if (mode != DPCM_SAMPLE) {
        if (inst_proc.get_sequ_enable(PIT_SEQU) || inst_proc.get_sequ_enable(HPI_SEQU)) {
            if (is_tone_mode(mode) || mode == FDS_WAVE || mode == N163_WAVE || is_vrc6_mode(mode)) {
                if (inst_proc.get_status(PIT_SEQU)) {
                    set_period(get_period() + (float)inst_proc.get_sequ_var(PIT_SEQU));
                }
                if (mode != FDS_WAVE && inst_proc.get_status(HPI_SEQU)) {
                    set_period(get_period() + (float)(inst_proc.get_sequ_var(HPI_SEQU) * 16));
                }
            } else if (is_noise_mode(mode)) {
                // DBG_PRINTF("PIT: %d->%d\n", inst_proc.get_pos(PIT_SEQU), inst_proc.get_sequ_var(PIT_SEQU));
                set_noise_rate((noise_rate_rel - inst_proc.get_sequ_var(PIT_SEQU)) & 15);
            }
        }

        if (inst_proc.get_sequ_enable(ARP_SEQU)) {
            if (inst_proc.get_status(ARP_SEQU)) {
                int8_t arp_value = inst_proc.get_sequ_var(ARP_SEQU);
                uint32_t arp_setting = inst_proc.get_sequ_setting(ARP_SEQU);
                if (is_tone_mode(mode) || mode == FDS_WAVE || mode == N163_WAVE || is_vrc6_mode(mode)) {
                    if (arp_setting == ARP_SETTING_FIXED) {
                        set_note_rely(fixed_arp_to_midi_note(arp_value));
                        arp_fixed_active = true;
                    } else if (arp_setting == ARP_SETTING_RELATIVE) {
                        arp_relative_note = clamp_midi_note(arp_relative_note + arp_value);
                        set_note_rely((uint8_t)arp_relative_note);
                        arp_fixed_active = false;
                    } else {
                        set_note_rely(clamp_midi_note((int)base_note + arp_value));
                        arp_fixed_active = false;
                    }
                } else if (is_noise_mode(mode)) {
                    // DBG_PRINTF("ARP: %d->%d\n", inst_proc.get_pos(ARP_SEQU), inst_proc.get_sequ_var(ARP_SEQU));
                    if (arp_setting == ARP_SETTING_FIXED) {
                        set_noise_rate_rel(note2noise(fixed_arp_to_midi_note(arp_value)));
                        arp_fixed_active = true;
                    } else if (arp_setting == ARP_SETTING_RELATIVE) {
                        arp_relative_note = clamp_midi_note(arp_relative_note + arp_value);
                        set_noise_rate_rel(note2noise((uint8_t)arp_relative_note));
                        arp_fixed_active = false;
                    } else {
                        set_noise_rate_rel((noise_rate + arp_value) & 15);
                        arp_fixed_active = false;
                    }
                }
            } else if (arp_fixed_active) {
                if (is_tone_mode(mode) || mode == FDS_WAVE || mode == N163_WAVE || is_vrc6_mode(mode)) {
                    set_note_rely(base_note);
                } else if (is_noise_mode(mode)) {
                    set_noise_rate_rel(noise_rate);
                }
                arp_fixed_active = false;
            }
        } else if (arp_fixed_active) {
            if (is_tone_mode(mode) || mode == FDS_WAVE || mode == N163_WAVE || is_vrc6_mode(mode)) {
                set_note_rely(base_note);
            } else if (is_noise_mode(mode)) {
                set_noise_rate_rel(noise_rate);
            }
            arp_fixed_active = false;
        }

        if (inst_proc.get_sequ_enable(DTY_SEQU)) {
            if (is_vrc6_pulse_mode(mode)) {
                set_mode((WAVE_TYPE)(VRC6_PULSE_062 + (inst_proc.get_sequ_var(DTY_SEQU) & 7)));
            } else if (mode == VRC6_SAW) {
                set_mode(VRC6_SAW);
            } else if (mode == N163_WAVE) {
                n163_wave_index = inst_proc.get_sequ_var(DTY_SEQU) & 0x0F;
                sync_n163_instrument();
            } else if (mode < TRIANGULAR) {
                set_mode((WAVE_TYPE)inst_proc.get_sequ_var(DTY_SEQU));
            } else if (is_noise_mode(mode)) {
                set_mode((WAVE_TYPE)(inst_proc.get_sequ_var(DTY_SEQU) & 1));
            }
        } else {
            set_mode(chl_mode);
        }
    }

    if (delay_cut_status) {
        if (delay_cut_count) {
            delay_cut_count--;
        } else {
            note_cut();
            delay_cut_status = false;
        }
    }
}

void FAMI_CHANNEL::render_tick_samples(size_t sample_count, bool advance_tick_phase) {
    tick_length = (int)sample_count;
    if (tick_buf.size() < sample_count) {
        tick_buf.resize(sample_count);
    }
    if (apu_level_buf.size() < sample_count) {
        apu_level_buf.resize(sample_count);
    }

    make_tick_sound(advance_tick_phase);
}

void FAMI_CHANNEL::end_tick_update() {
    if (mode != DPCM_SAMPLE) {
        if (slide_up) {
            set_period(get_period() + (is_direct_frequency_mode(mode) ? slide_up : -slide_up));
        } else if (slide_down) {
            set_period(get_period() + (is_direct_frequency_mode(mode) ? -slide_down : slide_down));
        }

        if (vol_slide_up) {
            vol_count++;
            if (vol_count >= vol_slide_up) {
                set_vol(get_vol() + 1);
                vol_count = 0;
            }
        }

        if (vol_slide_down) {
            vol_count++;
            if (vol_count >= vol_slide_down) {
                set_vol(get_vol() - 1);
                vol_count = 0;
            }
        }

        if (auto_port && !auto_port_finish) {
            if (auto_port_source > auto_port_target) {
                set_period(get_period() - auto_port);
                if (get_period() < auto_port_target) {
                    rely_note = base_note;
                    set_period(auto_port_target);
                    auto_port_finish = true;
                }
            } else if (auto_port_source < auto_port_target) {
                set_period(get_period() + auto_port);
                if (get_period() > auto_port_target) {
                    rely_note = base_note;
                    set_period(auto_port_target);
                    auto_port_finish = true;
                }
            }
        }

        if (portup_speed) {
            if (is_direct_frequency_mode(mode)) {
                set_period(get_period() + portup_speed);
                if (get_period() >= portup_target) {
                    portup_speed = 0;
                    set_period(portup_target);
                }
            } else {
                set_period(get_period() - portup_speed);
                if (get_period() <= portup_target) {
                    portup_speed = 0;
                    set_period(portup_target);
                }
            }
        } else if (portdown_speed) {
            if (is_direct_frequency_mode(mode)) {
                set_period(get_period() - portdown_speed);
                if (get_period() <= portdown_target) {
                    portdown_speed = 0;
                    set_period(portdown_target);
                }
            } else {
                set_period(get_period() + portdown_speed);
                if (get_period() >= portdown_target) {
                    portdown_speed = 0;
                    set_period(portdown_target);
                }
            }
        }

        if (arp_fx_n1 || arp_fx_n2) {
            if (arp_fx_pos == 0) {
                set_note_rely(base_note);
            } else if (arp_fx_pos == 1) {
                set_note_rely(base_note + arp_fx_n1);
            } else if (arp_fx_pos == 2) {
                set_note_rely(base_note + arp_fx_n2);
            }
            arp_fx_pos++;
            if (arp_fx_n2) {
                if (arp_fx_pos > 2) {
                    arp_fx_pos = 0;
                }
            } else {
                if (arp_fx_pos > 1) {
                    arp_fx_pos = 0;
                }
            }
        }
    }

    if (mode != VRC7_FM) {
        inst_proc.update_tick();
    }
}

void FAMI_CHANNEL::update_tick() {
    begin_tick_update();
    render_tick_samples((size_t)nominal_tick_length, true);
    end_tick_update();
}

void FAMI_CHANNEL::set_inst(int n) {
    // DBG_PRINTF("SET INST: %d\n", n);
    int old_inst = inst_proc.get_inst_num();
    inst_proc.set_inst(n);
    sync_vrc7_instrument();
    sync_fds_instrument();
    if (mode == N163_WAVE && n != old_inst) {
        n163_wave_index = 0;
    }
    sync_n163_instrument();
}

instrument_t *FAMI_CHANNEL::get_inst() {
    return inst_proc.get_inst();
}

void FAMI_CHANNEL::note_start() {
    arp_relative_note = base_note;
    arp_fixed_active = false;
    if (mode == DPCM_SAMPLE) {
        sample_phase = 0;
        sample_pos = sample_start * 512;
        sample_status = (sample_num >= 0 && sample_pos < sample_len);
    } else if (mode == VRC7_FM) {
        sync_vrc7_instrument();
        vrc7_gate = true;
        vrc7_release = false;
        vrc7_trigger = true;
        portup_speed = 0;
        portdown_speed = 0;
    } else if (mode == FDS_WAVE) {
        sync_fds_instrument();
        inst_proc.start();
        fds_gate = true;
        fds_phase = 0;
        fds_mod_counter = 0;
        portup_speed = 0;
        portdown_speed = 0;
    } else if (mode == N163_WAVE) {
        sync_n163_instrument();
        inst_proc.start();
        n163_gate = true;
        n163_phase = 0;
        portup_speed = 0;
        portdown_speed = 0;
    } else {
        inst_proc.start();
        portup_speed = 0;
        portdown_speed = 0;
    }
}

void FAMI_CHANNEL::set_dpcm_offset(uint8_t n) {
    sample_start = n;
    sample_pos = n * 512;
}

void FAMI_CHANNEL::set_dpcm_var(uint8_t n) {
    dmc_hold_level = n & 0x7f;
    sample_var = dmc_hold_level;
}

void FAMI_CHANNEL::set_dpcm_pitch(uint8_t n) {
    sample_pitch = n & 15;
}

void FAMI_CHANNEL::set_arp_fx(uint8_t n1, uint8_t n2) {
    arp_fx_n1 = n1;
    arp_fx_n2 = n2;
    arp_fx_pos = 0;

    slide_up = 0;
    slide_down = 0;
}

void FAMI_CHANNEL::note_end() {
    if (mode == DPCM_SAMPLE) {
        sample_var = dmc_hold_level;
        sample_status = false;
    } else if (mode == VRC7_FM) {
        vrc7_gate = false;
        vrc7_release = true;
        vrc7_trigger = true;
    } else if (mode == FDS_WAVE) {
        inst_proc.end();
    } else if (mode == N163_WAVE) {
        inst_proc.end();
    } else {
        inst_proc.end();
    }
}

void FAMI_CHANNEL::note_cut() {
    last_note = 255;
    arp_fixed_active = false;
    if (mode == DPCM_SAMPLE) {
        sample_var = dmc_hold_level;
        sample_status = false;
    } else if (mode == VRC7_FM) {
        vrc7_gate = false;
        vrc7_release = false;
        vrc7_trigger = true;
        portup_speed = 0;
        portdown_speed = 0;
    } else if (mode == FDS_WAVE) {
        fds_gate = false;
        inst_proc.cut();
        portup_speed = 0;
        portdown_speed = 0;
    } else if (mode == N163_WAVE) {
        n163_gate = false;
        inst_proc.cut();
        portup_speed = 0;
        portdown_speed = 0;
    } else {
        inst_proc.cut();
        portup_speed = 0;
        portdown_speed = 0;
    }
}

void FAMI_CHANNEL::set_period(float period_ref) {
    if (is_noise_mode(mode)) {
        set_noise_rate((uint8_t)period_ref & 15);
    } else if (mode == VRC7_FM) {
        period = period_ref;
        period_to_vrc7_pitch(period, vrc7_fnum, vrc7_block);
    } else if (mode == FDS_WAVE) {
        if (period_ref < 0.0f) period_ref = 0.0f;
        else if (period_ref > 4095.0f) period_ref = 4095.0f;
        period = period_ref;
    } else if (mode == N163_WAVE) {
        if (period_ref < 15.0f) period_ref = 15.0f;
        else if (period_ref > 65535.0f) period_ref = 65535.0f;
        period = period_ref;
    } else if (is_vrc6_mode(mode)) {
        if (period_ref < 12) period_ref = 12.0f;
        else if (period_ref > 4095) period_ref = 4095.0f;
        period = period_ref;
    } else {
        if (period_ref < 12) period_ref = 12.0f;
        else if (period_ref > 2048) period_ref = 2048.0f;
        period = period_ref;
    }
}

void FAMI_CHANNEL::set_freq(float freq_ref) {
    freq = freq_ref;
    period = (mode == N163_WAVE) ? n163_freq_to_period(freq, current_n163_channels(ftm_data)) : freq2period(freq);
}

void FAMI_CHANNEL::set_note_rely(uint8_t note) {
    if (is_noise_mode(mode)) {
        set_noise_rate(note2noise(note));
    } else if (mode == VRC7_FM) {
        rely_note = note;
        period = note2period(note);
        note_to_vrc7_pitch(note, vrc7_fnum, vrc7_block);
    } else if (mode == FDS_WAVE) {
        rely_note = note;
        period = note_to_fds_period(note);
    } else if (mode == N163_WAVE) {
        rely_note = note;
        period = note_to_n163_period(note, current_n163_channels(ftm_data));
    } else if (is_vrc6_mode(mode)) {
        rely_note = note;
        period = note2period(note);
    } else if (is_tone_mode(mode)) {
        rely_note = note;
        period = note2period(note);
    } else if (mode == DPCM_SAMPLE) {
        instrument_t *inst = inst_proc.get_inst();
        if (inst == NULL || note < 24 || note >= 120) {
            DBG_PRINTF("DPCM_CHANNEL: WARN -> NOTE_OUT_OF_RANGE %d\n", note);
            sample_num = -1;
            sample_len = 0;
            sample_status = false;
            return;
        }

        sample_num = inst->dpcm[note - 24].index - 1;
        if (sample_num < 0) {
            DBG_PRINTF("DPCM_CHANNEL: WARN -> NO_SAMPLE\n");
            sample_len = 0;
            sample_status = false;
            return;
        }
        if (sample_num >= (int)ftm_data->dpcm_samples.size()) {
            DBG_PRINTF("DPCM_CHANNEL: WARN -> SAMPLE_OUT_OF_RANGE %d\n", sample_num);
            sample_num = -1;
            sample_len = 0;
            sample_status = false;
            return;
        }

        sample_loop = inst->dpcm[note - 24].loop;
        sample_pitch = inst->dpcm[note - 24].pitch;
        sample_len = ftm_data->dpcm_samples[sample_num].sample_size_byte * 8;
        DBG_PRINTF("DPCM_CHANNEL: SET NUM->%d, PITCH->%d, LOOP->%d, SMP_LEN->%d\n", sample_num, sample_pitch, sample_loop, sample_len);
        sample_pos = sample_start * 256;
    }
}

float FAMI_CHANNEL::get_freq() {
    return freq;
}

float FAMI_CHANNEL::get_period() {
    if (mode == FDS_WAVE) {
        return period + period_offset;
    }
    if (mode == N163_WAVE) {
        return period;
    }
    return period - period_offset;
}

float FAMI_CHANNEL::get_period_rel() {
    if (is_tone_mode(mode) || is_vrc6_mode(mode))
        return period_rely;
    else if (is_noise_mode(mode))
        return get_noise_rate();
    else if (mode == VRC7_FM || mode == FDS_WAVE)
        return period;
    else if (mode == N163_WAVE)
        return n163_period_to_common_period(period, current_n163_channels(ftm_data));
    else
        return sample_num;
}

void FAMI_CHANNEL::set_noise_rate(uint8_t rate) {
    noise_rate = rate;
    set_noise_rate_rel(rate);
}

void FAMI_CHANNEL::set_noise_rate_rel(uint8_t rate) {
    noise_rate_rel = rate;
    update_noise_period();
}

uint8_t FAMI_CHANNEL::get_noise_rate() {
    return noise_rate_rel;
}

uint8_t FAMI_CHANNEL::get_dpcm_pitch() {
    return sample_pitch;
}

int FAMI_CHANNEL::get_dpcm_sample_num() {
    return sample_num;
}

uint8_t FAMI_CHANNEL::get_dpcm_sample_start() {
    return sample_start;
}

bool FAMI_CHANNEL::get_dpcm_sample_loop() {
    return sample_loop;
}

bool FAMI_CHANNEL::get_dpcm_sample_status() {
    return sample_status;
}

void FAMI_CHANNEL::set_mode(WAVE_TYPE m) {
    if (mode == VRC7_FM || m == VRC7_FM) {
        mode = VRC7_FM;
        chl_mode = VRC7_FM;
    } else if (mode == FDS_WAVE || m == FDS_WAVE) {
        mode = FDS_WAVE;
        chl_mode = FDS_WAVE;
    } else if (mode == N163_WAVE || m == N163_WAVE) {
        mode = N163_WAVE;
        chl_mode = N163_WAVE;
    } else if (mode == VRC6_SAW || m == VRC6_SAW) {
        mode = VRC6_SAW;
        chl_mode = VRC6_SAW;
    } else if (is_vrc6_pulse_mode(mode) || is_vrc6_pulse_mode(m)) {
        uint8_t duty = is_vrc6_pulse_mode(m) ? (uint8_t)(m - VRC6_PULSE_062) : ((uint8_t)m & 7);
        mode = (WAVE_TYPE)(VRC6_PULSE_062 + duty);
        chl_mode = mode;
    } else if (is_noise_mode(mode)) {
        mode = (WAVE_TYPE)((m&1) + NOISE0);
        // DBG_PRINTF("SET_NOISE_MODE: %d\n", mode);
    } else if (mode != TRIANGULAR) {
        mode = m;
    }
    // DBG_PRINTF("SET MODE: %d\n", mode);
}

void FAMI_CHANNEL::set_chl_mode(WAVE_TYPE m) {
    if (m == VRC7_FM || mode == VRC7_FM) {
        chl_mode = VRC7_FM;
        mode = VRC7_FM;
    } else if (m == FDS_WAVE || mode == FDS_WAVE) {
        chl_mode = FDS_WAVE;
        mode = FDS_WAVE;
    } else if (m == N163_WAVE || mode == N163_WAVE) {
        chl_mode = N163_WAVE;
        mode = N163_WAVE;
        if (m != N163_WAVE) {
            n163_wave_index = ((uint8_t)m) & 0x0F;
            sync_n163_instrument();
        }
    } else if (m == VRC6_SAW || mode == VRC6_SAW) {
        chl_mode = VRC6_SAW;
        mode = VRC6_SAW;
    } else if (is_vrc6_pulse_mode(m) || is_vrc6_pulse_mode(mode)) {
        uint8_t duty = is_vrc6_pulse_mode(m) ? (uint8_t)(m - VRC6_PULSE_062) : ((uint8_t)m & 7);
        chl_mode = (WAVE_TYPE)(VRC6_PULSE_062 + duty);
        mode = chl_mode;
    } else if (is_noise_mode(mode)) {
        chl_mode = (WAVE_TYPE)((m&1) + NOISE0);
        DBG_PRINTF("SET_NOISE_MODE(CHL): %d\n", chl_mode);
    } else if (mode != TRIANGULAR) {
        chl_mode = (WAVE_TYPE)(m & 3);
    }
}

WAVE_TYPE FAMI_CHANNEL::get_mode() {
    return mode;
}

void FAMI_CHANNEL::sync_vrc7_instrument() {
    if (mode != VRC7_FM) {
        return;
    }

    instrument_t *inst = inst_proc.get_inst();
    if (inst != NULL && inst->type == INST_VRC7) {
        vrc7_patch = inst->vrc7_patch & 0x0F;
        memcpy(vrc7_regs, inst->vrc7_regs, sizeof(vrc7_regs));
    } else {
        vrc7_patch = 0;
        memcpy(vrc7_regs, VRC7_DEFAULT_REGS, sizeof(vrc7_regs));
    }
}


void FAMI_CHANNEL::sync_fds_instrument() {
    if (mode != FDS_WAVE) {
        return;
    }

    instrument_t *inst = inst_proc.get_inst();
    if (inst != NULL && inst->type == INST_FDS) {
        memcpy(fds_wave, inst->fds_wave, sizeof(fds_wave));
        memcpy(fds_mod, inst->fds_mod, sizeof(fds_mod));
        fds_mod_speed = inst->fds_mod_speed;
        fds_mod_depth = inst->fds_mod_depth;
        fds_mod_delay = inst->fds_mod_delay;
    } else {
        instrument_t defaults;
        memcpy(fds_wave, defaults.fds_wave, sizeof(fds_wave));
        memset(fds_mod, 0, sizeof(fds_mod));
        fds_mod_speed = 0;
        fds_mod_depth = 0;
        fds_mod_delay = 0;
    }
}

void FAMI_CHANNEL::sync_n163_instrument() {
    if (mode != N163_WAVE) {
        return;
    }

    instrument_t *inst = inst_proc.get_inst();
    if (inst != NULL && inst->type == INST_N163) {
        uint8_t count = inst->n163_wave_count;
        if (count < 1 || count > FAMI32_N163_WAVE_MAX) {
            count = 1;
        }
        if (n163_wave_index >= count) {
            n163_wave_index = count - 1;
        }
        n163_wave_size = inst->n163_wave_size;
        if (n163_wave_size < 1 || n163_wave_size > FAMI32_N163_WAVE_SIZE) {
            n163_wave_size = FAMI32_N163_WAVE_SIZE;
        }
        memcpy(n163_wave, inst->n163_wave[n163_wave_index], sizeof(n163_wave));
    } else {
        instrument_t defaults;
        n163_wave_size = FAMI32_N163_WAVE_SIZE;
        n163_wave_index = 0;
        memcpy(n163_wave, defaults.n163_wave[0], sizeof(n163_wave));
    }
}

void FAMI_CHANNEL::set_vrc7_channel(uint8_t n) {
    vrc7_channel_index = n % FAMI32_VRC7_CHANNELS;
}

uint8_t FAMI_CHANNEL::get_vrc7_channel() const {
    return vrc7_channel_index;
}

bool FAMI_CHANNEL::is_vrc7_mode() const {
    return mode == VRC7_FM;
}

bool FAMI_CHANNEL::get_vrc7_gate() const {
    return vrc7_gate;
}

bool FAMI_CHANNEL::get_vrc7_release() const {
    return vrc7_release;
}

bool FAMI_CHANNEL::consume_vrc7_trigger() {
    bool out = vrc7_trigger;
    vrc7_trigger = false;
    return out;
}

uint8_t FAMI_CHANNEL::get_vrc7_patch() const {
    return vrc7_patch & 0x0F;
}

uint8_t FAMI_CHANNEL::get_vrc7_reg(uint8_t reg) const {
    return vrc7_regs[reg & 7];
}

uint16_t FAMI_CHANNEL::get_vrc7_fnum() const {
    return vrc7_fnum;
}

uint8_t FAMI_CHANNEL::get_vrc7_block() const {
    return vrc7_block & 7;
}

uint8_t FAMI_CHANNEL::get_vrc7_volume() const {
    uint8_t volume = chl_vol > 15 ? 15 : chl_vol;
    return 15 - volume;
}

bool FAMI_CHANNEL::is_fds_mode() const {
    return mode == FDS_WAVE;
}

bool FAMI_CHANNEL::get_fds_gate() const {
    return fds_gate;
}

bool FAMI_CHANNEL::is_n163_mode() const {
    return mode == N163_WAVE;
}

bool FAMI_CHANNEL::get_n163_gate() const {
    return n163_gate;
}

void FAMI_CHANNEL::set_fds_mod_depth(uint8_t depth) {
    if (mode != FDS_WAVE) return;
    fds_mod_depth = depth & 0x3F;
}

void FAMI_CHANNEL::set_fds_mod_speed_hi(uint8_t value) {
    if (mode != FDS_WAVE) return;
    fds_mod_speed = (fds_mod_speed & 0x00FF) | ((uint32_t)(value & 0x0F) << 8);
}

void FAMI_CHANNEL::set_fds_mod_speed_lo(uint8_t value) {
    if (mode != FDS_WAVE) return;
    fds_mod_speed = (fds_mod_speed & 0x0F00) | value;
}

void FAMI_CHANNEL::set_note(uint8_t note) {
    if (auto_port) {
        auto_port_target = note_to_channel_period(mode, note, current_n163_channels(ftm_data));
        if (last_note == 255) {
            set_note_rely(note);
            auto_port_source = get_period();
            auto_port_source = auto_port_target;
        } else {
            auto_port_source = get_period();
        }
        auto_port_finish = false;
        // printf("AUTO_PORT (P=%d): %f -> %f\n", last_note, auto_port_source, auto_port_target);
    } else {
        set_note_rely(note);
    }
    last_note = base_note;
    base_note = note;
    arp_relative_note = note;
    // DBG_PRINTF("SET_NOTE: P=%f, F=%f\n", period, freq);
}

void FAMI_CHANNEL::set_vol(int8_t vol) {
    if (vol < 0)
        vol = 0;
    else if (vol > 15)
        vol = 15;

    chl_vol = vol;
}

int8_t FAMI_CHANNEL::get_vol() {
    return chl_vol;
}

int8_t FAMI_CHANNEL::get_env_vol() {
    return inst_proc.get_sequ_var(VOL_SEQU);
}

uint8_t FAMI_CHANNEL::get_rel_vol() {
    if (mode == DPCM_SAMPLE) {
        return dmc_hold_level * 2;
    } else {
        return rel_vol;
    }
}

int16_t* FAMI_CHANNEL::get_buf() {
    return tick_buf.data();
}

size_t FAMI_CHANNEL::get_buf_size_byte() {
    return tick_buf.size() * sizeof(int16_t);
}

size_t FAMI_CHANNEL::get_buf_size() {
    return tick_buf.size();
}

uint8_t* FAMI_CHANNEL::get_apu_level_buf() {
    return apu_level_buf.data();
}

size_t FAMI_CHANNEL::get_apu_level_buf_size_byte() {
    return apu_level_buf.size() * sizeof(uint8_t);
}

int FAMI_CHANNEL::get_samp_pos() {
    return sample_pos;
}

size_t FAMI_CHANNEL::get_samp_len() {
    return sample_len;
}

uint32_t FAMI_CHANNEL::get_inst_pos(int sequ_type) {
    return inst_proc.get_pos(sequ_type);
}
