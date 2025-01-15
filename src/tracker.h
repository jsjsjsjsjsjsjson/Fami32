#ifndef TRACKER_H
#define TRACKER_H

#include <stdio.h>
#include <vector>

#include "ftm_file.h"

#include "note2freq.h"

#include "wave_table.h"
#include "src_config.h"

#include "basic_dsp.h"

bool _debug_print = false;

#define DBG_PRINTF(fmt, ...) \
    do { \
        if (_debug_print) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define VOL_SEQU 0
#define ARP_SEQU 1
#define PIT_SEQU 2
#define HPI_SEQU 3
#define DTY_SEQU 4

typedef enum {
    SEQU_STOP,
    SEQU_PLAYING,
    SEQU_RELEASE
} note_stat_t;

class FAMI_INSTRUMENT {
private:
    FTM_FILE *ftm_data;
    uint32_t pos[5];
    int8_t var[5];
    note_stat_t status[5];
    int inst_num = 0;
    instrument_t *instrument;

public:
    void init(FTM_FILE* data) {
        memset(pos, 0, sizeof(pos));
        memset(status, 0, sizeof(pos));
        ftm_data = data;
        instrument = &ftm_data->instrument[0];
        DBG_PRINTF("INIT FTM_FILE IN %p\n", ftm_data);
    }

    void set_inst(int n) {
        memset(pos, 0, sizeof(pos));
        memset(status, 0, sizeof(pos));
        inst_num = n;
        // DBG_PRINTF("(DEBUG)INSTRUMENT_SIZE: %d\n", ftm_data->instrument.size());
        instrument = &ftm_data->instrument[inst_num];
    }

    int get_inst_num() {
        return inst_num;
    }

    instrument_t* get_inst() {
        return instrument;
    }

    void start() {
        memset(pos, 0, sizeof(pos));
        for (int i = 0; i < 5; i++) {
            if (instrument->seq_index[i].enable) {
                status[i] = SEQU_PLAYING;
                var[i] = ftm_data->sequences[i][instrument->seq_index[i].seq_index].data[0];
            } else {
                status[i] = SEQU_STOP;
                var[i] = i ? 0 : 15;
            }
        }
    }

    void end() {
        for (int i = 0; i < 5; i++) {
            if (instrument->seq_index[i].enable) {
                status[i] = SEQU_RELEASE;
            } else {
                status[i] = SEQU_STOP;
                var[i] = 0;
            }
        }
    }

    void cut() {
        memset(pos, 0, sizeof(pos));
        for (int i = 0; i < 5; i++) {
            status[i] = SEQU_STOP;
            var[i] = 0;
        }
    }

    void update_tick() {
        for (int i = 0; i < 5; i++) {
            if (status[i]) {
                sequences_t *sequ = &ftm_data->sequences[i][instrument->seq_index[i].seq_index];
                pos[i]++;
                if (status[i] == SEQU_RELEASE) {
                    if (sequ->release == SEQ_FEAT_DISABLE) {
                        status[i] = SEQU_STOP;
                        if (i == 0) {
                            var[0] = 0;
                            continue;
                        }
                    } else if (pos[i] > sequ->release) {
                        pos[i]--;
                    }
                }
                if (pos[i] >= sequ->length) {
                    pos[i]--;
                    if (sequ->loop == SEQ_FEAT_DISABLE) {
                        status[i] = SEQU_STOP;
                    } else {
                        pos[i] = sequ->loop;
                    }
                }
                var[i] = sequ->data[pos[i]];
            }
        }
    }

    int8_t get_sequ_var(int n) {
        return var[n];
    }

    uint32_t get_pos(int n) {
        return pos[n];
    }

    note_stat_t get_status(int n) {
        return status[n];
    }

    bool get_sequ_enable(int n) {
        return instrument->seq_index[n].enable;
    }
};

class FAMI_CHANNEL {
private:
    FTM_FILE *ftm_data;
    FAMI_INSTRUMENT inst_proc;

    int i_pos;
    float f_pos;

    float pos_count;

    float freq;
    float period;

    int8_t chl_vol = 15;

    uint8_t base_note;
    uint8_t rely_note;

    uint8_t noise_rate = 0;
    uint8_t noise_rate_rel = 0;

    int sample_pos = 0;
    float sample_fpos = 0.0f;
    uint8_t sample_pitch = 0;
    int sample_num = 0;
    bool sample_status = false;

    uint8_t sample_start = 0;

    int8_t sample_loop = false;

    size_t sample_len = 0;

    int8_t sample_var = 0;

    int tick_length = SAMP_RATE / ENG_SPEED;
    std::vector<int16_t> tick_buf;

    WAVE_TYPE mode;
    WAVE_TYPE chl_mode;

    //FX Flag
    uint8_t slide_up = 0;
    uint8_t slide_down = 0;

    uint8_t vol_slide_up = 0;
    uint8_t vol_slide_down = 0;

    uint8_t vol_count = 0;

    uint8_t auto_port = 0;
    float auto_port_source;
    float auto_port_target;
    bool auto_port_finish = true;

    uint8_t vib_spd = 0;
    uint8_t vib_dep = 0;

    uint8_t vib_pos = 0;

    float portup_target;
    uint8_t portup_speed = 0;

    float portdown_target;
    uint8_t portdown_speed = 0;

    int8_t period_offset = 0;

    uint8_t delay_cut_count = 0;
    bool delay_cut_status = false;

    uint8_t arp_fx_n1;
    uint8_t arp_fx_n2;

    uint8_t arp_fx_pos = 0;

    uint8_t rel_vol;

public:

    void clear_all_fx_flag() {
        chl_vol = 15;
        rel_vol = 0;
        period = 0;
        freq = 0;
        pos_count = 0;

        if (mode < TRIANGULAR) {
            mode = PULSE_125;
        } else if (mode > DPCM_SAMPLE) {
            mode = NOISE0;
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
    }

    void init(FTM_FILE* data) {
        ftm_data = data;
        inst_proc.init(data);
        tick_buf.resize(tick_length);
        DBG_PRINTF("INIT: ftm_data = %p, tick_length = (%d / %d) = %d\n", ftm_data, SAMP_RATE, ENG_SPEED, tick_length);
    }

    void set_slide_up(uint8_t n) {
        arp_fx_n1 = 0;
        arp_fx_n2 = 0;

        slide_up = n;
        slide_down = 0;
        auto_port = 0;
    }

    void set_slide_down(uint8_t n) {
        arp_fx_n1 = 0;
        arp_fx_n2 = 0;

        slide_down = n;
        slide_up = 0;
        auto_port = 0;
    }

    void set_vol_slide_up(uint8_t n) {
        vol_count = 0;
        if (n)
            vol_slide_up = roundf(8.0f / n);
        else
            vol_slide_up = 0;
    }

    void set_vol_slide_down(uint8_t n) {
        vol_count = 0;
        if (n)
            vol_slide_down = roundf(8.0f / n);
        else
            vol_slide_down = 0;
    }

    void set_auto_port(uint8_t n) {
        arp_fx_n1 = 0;
        arp_fx_n2 = 0;

        auto_port = n;
        auto_port_source = get_period();
        auto_port_finish = true;
        slide_up = 0;
        slide_down = 0;
    }

    void set_vibrato(uint8_t spd, uint8_t dep) {
        vib_spd = spd;
        vib_dep = dep;
    }

    void set_period_offset(int8_t var) {
        period_offset = var;
        set_period(get_period());
    }

    void set_port_up(uint8_t spd, uint8_t offset) {
        arp_fx_n1 = 0;
        arp_fx_n2 = 0;

        slide_up = 0;
        slide_down = 0;

        portdown_speed = 0;
        auto_port = 0;
        portup_speed = (spd * 2) + 1;
        portup_target = note2period(base_note + offset);
        base_note += offset;
    }

    void set_port_down(uint8_t spd, uint8_t offset) {
        arp_fx_n1 = 0;
        arp_fx_n2 = 0;

        slide_up = 0;
        slide_down = 0;

        portup_speed = 0;
        auto_port = 0;
        portdown_speed = (spd * 2) + 1;
        portdown_target = note2period(base_note - offset);
        base_note -= offset;
    }

    void set_delay_cut(uint8_t t) {
        delay_cut_count = t;
        delay_cut_status = true;
    }

    void make_tick_sound() {
        if (mode < 5) {
            int8_t env_vol = inst_proc.get_sequ_var(VOL_SEQU);
            if (mode == TRIANGULAR) {
                env_vol = env_vol ? 15 : 0;
                chl_vol = chl_vol ? 15 : 0;
            }
            rel_vol = env_vol * chl_vol;
            // DBG_PRINTF("(DEBUG: VOL) GET_ENV: %d->%d\n", inst_proc.get_pos(VOL_SEQU), inst_proc.get_sequ_var(VOL_SEQU));
            for (int i = 0; i < tick_length; i++) {
                i_pos = i_pos & 31;
                tick_buf[i] = wave_table[mode][i_pos] * rel_vol;
                if (!env_vol || !chl_vol) {
                    continue;
                }
                if (mode == 4) {
                    f_pos += pos_count / 2;
                }
                else {
                    f_pos += pos_count;
                }
                if (f_pos > 1.0f) {
                    i_pos += (int)f_pos;
                    f_pos -= (int)f_pos;
                }
            }
        } else if (mode > 5) {
            int8_t env_vol = inst_proc.get_sequ_var(VOL_SEQU);
            rel_vol = env_vol * chl_vol;
            for (int i = 0; i < tick_length; i++) {
                tick_buf[i] = nes_noise_make(mode - 6) * rel_vol;
            }
        } else if (mode == DPCM_SAMPLE) {
            float count = dpcm_pitch_table[sample_pitch] / SAMP_RATE;
            if (sample_num < ftm_data->dpcm_samples.size()) {
                int8_t *sample = ftm_data->dpcm_samples[sample_num].pcm_data.data();
                for (int i = 0; i < tick_length; i++) {
                    if (sample_status) {
                        sample_var = sample[sample_pos];
                        sample_fpos += count;
                        if (sample_fpos > 1.0f) {
                            sample_pos += (int)sample_fpos;
                            sample_fpos -= (int)sample_fpos;
                            if (sample_pos >= sample_len) {
                                if (sample_loop) {
                                    sample_pos = 0;
                                } else {
                                    sample_fpos = 0;
                                    sample_pos = sample_start * 8;
                                    sample_status = false;
                                }
                            }
                        }
                    }
                    tick_buf[i] = sample_var * 128;
                }
            }
        } else {
            memset(tick_buf.data(), 0, tick_buf.size() * sizeof(int16_t));
        }
    }

    void update_tick() {
        if (mode != DPCM_SAMPLE) {
            if (inst_proc.get_sequ_enable(PIT_SEQU) || inst_proc.get_sequ_enable(HPI_SEQU)) {
                if (mode < 5) {
                    set_period(get_period()
                                + (float)inst_proc.get_sequ_var(PIT_SEQU) +
                                (float)(inst_proc.get_sequ_var(HPI_SEQU) * 16));
                } else if (mode > 5) {
                    // DBG_PRINTF("PIT: %d->%d\n", inst_proc.get_pos(PIT_SEQU), inst_proc.get_sequ_var(PIT_SEQU));
                    set_noise_rate((noise_rate_rel - inst_proc.get_sequ_var(PIT_SEQU)) & 15);
                }
            }

            if (inst_proc.get_sequ_enable(ARP_SEQU)) {
                if (mode < 5) {
                    set_note_rely(base_note + inst_proc.get_sequ_var(ARP_SEQU));
                } else if (mode > 5) {
                    // DBG_PRINTF("ARP: %d->%d\n", inst_proc.get_pos(ARP_SEQU), inst_proc.get_sequ_var(ARP_SEQU));
                    set_noise_rate_rel((noise_rate + inst_proc.get_sequ_var(ARP_SEQU)) & 15);
                }
            }

            if (inst_proc.get_sequ_enable(DTY_SEQU)) {
                if (mode < 4) {
                    set_mode((WAVE_TYPE)inst_proc.get_sequ_var(DTY_SEQU));
                } else if (mode > 5) {
                    set_mode((WAVE_TYPE)(inst_proc.get_sequ_var(DTY_SEQU) & 1));
                }
            } else {
                set_mode(chl_mode);
            }
        }

        make_tick_sound();

        if (mode != DPCM_SAMPLE) {
            if (slide_up) {
                set_period(get_period() - slide_up);
            } else if (slide_down) {
                set_period(get_period() + slide_down);
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
                set_period(get_period() - portup_speed);
                if (get_period() < portup_target) {
                    portup_speed = 0;
                    set_period(portup_target);
                }
            } else if (portdown_speed) {
                set_period(get_period() + portdown_speed);
                if (get_period() > portdown_target) {
                    portdown_speed = 0;
                    set_period(portdown_target);
                }
            }

            if (vib_spd && vib_dep) {
                vib_pos = (vib_pos + vib_spd) & 63;
                int8_t vib_var = (vib_table[vib_pos] * vib_dep) / 16;
                freq = period2freq(get_period() + vib_var);
                pos_count = (freq / SAMP_RATE) * 32;
            } else {
                vib_pos = 0;
            }

            if (arp_fx_n1 || arp_fx_n2) {
                if (arp_fx_pos) {
                    set_note_rely(base_note + arp_fx_n2);
                } else {
                    set_note_rely(base_note + arp_fx_n1);
                }
                arp_fx_pos = !arp_fx_pos;
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

        inst_proc.update_tick();
    }

    void set_inst(int n) {
        // DBG_PRINTF("SET INST: %d\n", n);
        inst_proc.set_inst(n);
    }

    void note_start() {
        if (mode == DPCM_SAMPLE) {
            sample_fpos = 0;
            sample_pos = sample_start * 8;
            sample_status = true;
        } else {
            inst_proc.start();
            portup_speed = 0;
            portdown_speed = 0;
            // i_pos = 8;
        }
    }

    void set_dpcm_offset(uint8_t n) {
        sample_start = n;
    }

    void set_dpcm_var(uint8_t n) {
        sample_var = n;
    }

    void set_arp_fx(uint8_t n1, uint8_t n2) {
        arp_fx_n1 = n1;
        arp_fx_n2 = n2;
        arp_fx_pos = 0;
    }

    void note_end() {
        if (mode == DPCM_SAMPLE) {
            sample_var = 0;
            sample_status = false;
        } else {
            inst_proc.end();
        }
    }

    void note_cut() {
        if (mode == DPCM_SAMPLE) {
            sample_var = 0;
            sample_status = false;
        } else {
            inst_proc.cut();
            auto_port = false;
            auto_port_finish = true;
            portup_speed = false;
            portdown_speed = false;
        }
    }

    void set_period(float period_ref) {
        if (mode > 5) {
            set_noise_rate((uint8_t)period_ref & 15);
        } else {
            period = period_ref;
            freq = period2freq(period - period_offset);
            // printf("PERIOD_OFF: %.1f\n", period_offset);
            pos_count = (freq / SAMP_RATE) * 32;
        }
        // DBG_PRINTF("SET_PERIOD: P=%f, F=%f, C=%f\n", period, freq, pos_count);
    }

    void set_freq(float freq_ref) {
        freq = freq_ref;
        period = freq2period(freq);
        pos_count = (freq / SAMP_RATE) * 32;
    }

    void set_note_rely(uint8_t note) {
        if (mode > 5) {
            set_noise_rate(note2noise(note));
        } else if (mode < 5) {
            rely_note = note;
            period = note2period(note);
            freq = period2freq(period - period_offset);
            pos_count = (freq / SAMP_RATE) * 32;
        } else if (mode == DPCM_SAMPLE) {
            sample_num = inst_proc.get_inst()->dpcm[note - 24].index - 1;
            if (sample_num < 0) {
                DBG_PRINTF("DPCM_CHANNEL: WARN -> NO_SAMPLE\n");
                return;
            }
            sample_loop = inst_proc.get_inst()->dpcm[note - 24].loop;
            sample_pitch = inst_proc.get_inst()->dpcm[note - 24].pitch;
            sample_len = ftm_data->dpcm_samples[sample_num].sample_size_byte * 8;
            DBG_PRINTF("DPCM_CHANNEL: SET NUM->%d, PITCH->%d, LOOP->%d, SMP_LEN->%d\n", sample_num, sample_pitch, sample_loop, sample_len);
            sample_pos = 0;
        }
    }

    float get_freq() {
        return freq;
    }

    float get_period() {
        return period - period_offset;
    }

    void set_noise_rate(uint8_t rate) {
        noise_rate = rate;
        set_noise_rate_rel(rate);
    }

    void set_noise_rate_rel(uint8_t rate) {
        noise_rate_rel = rate;
        update_noise_freq(noise_freq_table[0][noise_rate_rel]);
    }

    uint8_t get_noise_rate() {
        return noise_rate_rel;
    }

    void set_dpcm_pitch(uint8_t pit) {
        sample_pitch = pit;
    }

    uint8_t get_dpcm_pitch() {
        return sample_pitch;
    }

    void set_mode(WAVE_TYPE m) {
        if (mode > DPCM_SAMPLE) {
            mode = (WAVE_TYPE)((m&1) + NOISE0);
            // DBG_PRINTF("SET_NOISE_MODE: %d\n", mode);
        } else if (mode != TRIANGULAR) {
            mode = m;
        }
        // DBG_PRINTF("SET MODE: %d\n", mode);
    }

    void set_chl_mode(WAVE_TYPE m) {
        if (mode > DPCM_SAMPLE) {
            chl_mode = (WAVE_TYPE)((m&1) + NOISE0);
            DBG_PRINTF("SET_NOISE_MODE(CHL): %d\n", chl_mode);
        } else if (mode != TRIANGULAR) {
            chl_mode = m;
        }
    }

    WAVE_TYPE get_mode() {
        return mode;
    }

    void set_note(uint8_t note) {
        base_note = note;
        if (auto_port) {
            auto_port_source = get_period();
            auto_port_target = note2period(note);
            auto_port_finish = false;
        } else {
            set_note_rely(note);
        }
        // DBG_PRINTF("SET_NOTE: P=%f, F=%f, C=%f\n", period, freq, pos_count);
    }

    void set_vol(int8_t vol) {
        if (vol < 0)
            vol = 0;
        else if (vol > 15)
            vol = 15;

        chl_vol = vol;
    }

    int8_t get_vol() {
        return chl_vol;
    }

    int8_t get_env_vol() {
        return inst_proc.get_sequ_var(VOL_SEQU);
    }

    uint8_t get_rel_vol() {
        if (mode == DPCM_SAMPLE) {
            return abs(sample_var) * 2;
        } else {
            return rel_vol;
        }
    }

    int16_t* get_buf() {
        return tick_buf.data();
    }

    size_t get_buf_size_byte() {
        return tick_buf.size() * sizeof(int16_t);
    }

    size_t get_buf_size() {
        return tick_buf.size();
    }
};

class FAMI_PLAYER {
private:
    FTM_FILE *ftm_data;

    int row = 0;
    int frame = 0;

    float tick_accumulator = 0.0f;

    int speed = 150;
    int tempo = 3;

    float ticks_row = 0.0f;

    std::vector<int16_t> buf;

    uint8_t delay_count[5] = {0, 0, 0, 0, 0};
    bool delay_status[5] = {0, 0, 0, 0, 0};

    unpk_item_t ft_items[5];

    LowPassFilter lpf;
    HighPassFilter hpf;

    bool play_status = false;

public:
    FAMI_CHANNEL channel[5];

    void init(FTM_FILE* data) {
        ftm_data = data;
        for (int c = 0; c < 5; c++) {
            channel[c].init(ftm_data);
        }

        channel[2].set_mode(TRIANGULAR);
        channel[3].set_mode(NOISE0);

        channel[4].set_mode(DPCM_SAMPLE);

        buf.resize(channel[0].get_buf_size());

        lpf.setCutoffFrequency(18000, SAMP_RATE);
        hpf.setCutoffFrequency(32, SAMP_RATE);
    }

    void reload() {
        stop_play();

        set_speed(ftm_data->fr_block.speed);
        set_tempo(ftm_data->fr_block.tempo);
        for (int c = 0; c < 5; c++) {
            channel[c].init(ftm_data);
        }

        frame = 0;
        row = 0;
    }

    void start_play() {
        play_status = true;
        row = 0;
        // frame = 0;
    }

    void stop_play() {
        play_status = false;
        for (int c = 0; c < 5; c++) {
            channel[c].clear_all_fx_flag();
            channel[c].note_cut();
            memset(channel[c].get_buf(), 0, channel[c].get_buf_size_byte());
        }
        memset(buf.data(), 0, get_buf_size_byte());
    }

    void set_speed(int speed_ref) {
        if (speed_ref < 1) speed_ref = 1;
        speed = speed_ref;
        recalculate_ticks_row();
    }

    void set_tempo(int tempo_ref) {
        tempo = tempo_ref;
        recalculate_ticks_row();
    }

    int get_speed() const {
        return speed;
    }

    int get_tempo() const {
        return tempo;
    }

    void set_ticks_row(float tr) {
        ticks_row = tr;
    }

    float get_ticks_row() const {
        return ticks_row;
    }

    void mix_all_channel() {
        for (int c = 0; c < 5; c++) {
            channel[c].update_tick();
        }
        for (size_t i = 0; i < buf.size(); i++) {
            int32_t r = 0;
            for (int c = 0; c < 5; c++) {
                r += channel[c].get_buf()[i];
            }

            r = hpf.process(r);
            r = lpf.process(r);
            buf[i] = r / 2;
        }
    }

    uint8_t get_chl_vol(int n) {
        return channel[n].get_vol();
    }

    int8_t get_chl_env_vol(int n) {
        return channel[n].get_env_vol();
    }

    void process_delay_efx(fx_t fxdata[4], int c) {
        uint8_t count = ftm_data->ch_fx_count(c);
        for (uint8_t i = 0; i < count; i++) {
            if (fxdata[i].fx_cmd == 0x0E) {
                if (fxdata[i].fx_var >= ticks_row) {
                    DBG_PRINTF("C%d: WARN DELAY OUTOF TICKSROW\n", c);
                    delay_count[c] = (uint8_t)ticks_row;
                    delay_status[c] = true;
                } else {
                    delay_count[c] = fxdata[i].fx_var;
                    delay_status[c] = true;
                    DBG_PRINTF("C%d: SET DELAY -> %d\n", c, delay_count[c]);
                }
            }
        }
    }

    void process_efx(fx_t fxdata[4], int c) {
        uint8_t count = ftm_data->ch_fx_count(c);
        for (uint8_t i = 0; i < count; i++) {
            if (fxdata[i].fx_cmd == 0x01) {
                if (fxdata[i].fx_var >= 0x20) {
                    set_tempo(fxdata[i].fx_var);
                    DBG_PRINTF("C%d: SET TEMPO -> %d\n", c, fxdata[i].fx_var);
                } else {
                    set_speed(fxdata[i].fx_var);
                    DBG_PRINTF("C%d: SET SPEED -> %d\n", c, fxdata[i].fx_var);
                }
            } else if (fxdata[i].fx_cmd == 0x02) {
                jmp_to_frame(fxdata[i].fx_var);
                DBG_PRINTF("C%d: JMP TO FRAME -> %02X\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x03) {
                next_frame(fxdata[i].fx_var - 1);
                DBG_PRINTF("C%d: SKIP TO NEXT FRAME ROW %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x06) {
                channel[c].set_auto_port(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET AUTO_PORT -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x0A) {
                channel[c].set_arp_fx(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
                DBG_PRINTF("C%d: SET ARP -> N1 +%d, N2 +%d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            } else if (fxdata[i].fx_cmd == 0x0B) {
                channel[c].set_vibrato(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
                DBG_PRINTF("C%d: SET VIBRATO SPEED -> %d, DEEP -> %d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            } else if (fxdata[i].fx_cmd == 0x0D) {
                channel[c].set_period_offset(fxdata[i].fx_var - 0x80);
                DBG_PRINTF("C%d: SET PERIOD_OFFSET -> %d\n", c, fxdata[i].fx_var - 0x80);
            } else if (fxdata[i].fx_cmd == 0x0F) {
                channel[c].set_dpcm_var(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET DPCM VAR -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x10) {
                channel[c].set_slide_up(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET SLIDE_UP -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x11) {
                channel[c].set_slide_down(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET SLIDE_DOWN -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x12) {
                channel[c].set_chl_mode((WAVE_TYPE)(fxdata[i].fx_var));
                DBG_PRINTF("C%d: SET MODE -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x13) {
                channel[c].set_dpcm_offset(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET DPCM OFFSET %d Bytes (%d Samples)\n", c, fxdata[i].fx_var, fxdata[i].fx_var * 8);
            } else if (fxdata[i].fx_cmd == 0x14) {
                channel[c].set_port_up(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
                DBG_PRINTF("C%d: SET PORT_UP SPEED -> %d, +%dNOTE\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            } else if (fxdata[i].fx_cmd == 0x15) {
                channel[c].set_port_down(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
                DBG_PRINTF("C%d: SET PORT_DOWN SPEED -> %d, +%dNOTE\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            } else if (fxdata[i].fx_cmd == 0x16) {
                channel[c].set_vol_slide_up(HEX_B1(fxdata[i].fx_var));
                channel[c].set_vol_slide_down(HEX_B2(fxdata[i].fx_var));
                DBG_PRINTF("C%d: SET VOL_SLIDE, UP->%d - DOWN->%d = %d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var), HEX_B1(fxdata[i].fx_var) - HEX_B2(fxdata[i].fx_var));
            } else if (fxdata[i].fx_cmd == 0x17) {
                channel[c].set_delay_cut(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET DELAY_CUT -> %d\n", fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x0E) {
                // DBG_PRINTF("");
            } else if (fxdata[i].fx_cmd != NO_EFX) {
                DBG_PRINTF("UNKNOW CMD: %02X, PARAM=%02X\n", fxdata[i].fx_cmd, fxdata[i].fx_var);
            }
        }
    }

    void process_item(unpk_item_t item, int c) {
        process_efx(item.fxdata, c);
        if (item.instrument != NO_INST) {
            channel[c].set_inst(item.instrument);
        }
        if (item.note != NO_NOTE) {
            if (item.note == NOTE_END) {
                channel[c].note_end();
            } else if (item.note == NOTE_CUT) {
                channel[c].note_cut();
            } else {
                channel[c].set_note(item2note(item.note, item.octave));
                channel[c].note_start();
            }
        }
        if (item.volume != NO_VOL) {
            channel[c].set_vol(item.volume);
            // DBG_PRINTF("SET_VOL(C%d): %d\n", c, channel[c].get_vol());
        }
    }

    void process_tick() {
        // printf("PROCESS TICK, STATUS -> %d\n", play_status);
        if (!play_status) {
            return;
        }
        for (int c = 0; c < 5; c++) {
            if (!delay_status[c]) {
                continue;
            }
            if (delay_count[c]) {
                delay_count[c]--;
                if (!delay_count[c]) {
                    process_item(ft_items[c], c);
                    delay_status[c] = false;
                }
            }
        }

        while (tick_accumulator >= ticks_row) {
            for (int c = 0; c < 5; c++) {
                ft_items[c] = ftm_data->get_pt_item(c, ftm_data->get_frame_map(frame, c), row);
                // process_efx(ft_items[c].fxdata, c);
                process_delay_efx(ft_items[c].fxdata, c);
                if (!delay_count[c])
                    process_item(ft_items[c], c);
            }

            row++;
            if (row >= ftm_data->fr_block.pat_length) {
                next_frame(0);
            }

            tick_accumulator -= ticks_row;
        }

        tick_accumulator += 1.0f;
        mix_all_channel();
    }

    void next_frame(int r) {
        row = r;
        frame++;
        while (frame >= ftm_data->fr_block.frame_num) {
            frame -= ftm_data->fr_block.frame_num;
        }
    }

    void jmp_to_frame(int f) {
        row = -1;
        frame = f;
        while (frame >= ftm_data->fr_block.frame_num) {
            frame -= ftm_data->fr_block.frame_num;
        }
    }

    int16_t* get_buf() {
        return buf.data();
    }

    size_t get_buf_size_byte() const {
        return buf.size() * sizeof(int16_t);
    }

    size_t get_buf_size() const {
        return buf.size();
    }

    int get_row() {
        return row;
    }

    int get_frame() {
        return frame;
    }

    uint8_t get_cur_frame_map(int c) {
        return ftm_data->get_frame_map(frame, c);
    }

    bool get_play_status() {
        return play_status;
    }

private:
    void recalculate_ticks_row() {
        ticks_row = (float)(150 * speed) / (float)tempo;
        tick_accumulator = ticks_row;
    }
};

#endif // TRACKER_H