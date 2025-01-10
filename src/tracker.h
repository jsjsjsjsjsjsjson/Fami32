#ifndef TRACKER_H
#define TRACKER_H

#include <stdio.h>
#include <vector>

#include "ftm_file.h"

#include "note2freq.h"

#include "wave_table.h"
#include "src_config.h"

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
        printf("INIT FTM_FILE IN %p\n", ftm_data);
    }

    void set_inst(int n) {
        memset(pos, 0, sizeof(pos));
        memset(status, 0, sizeof(pos));
        inst_num = n;
        // printf("(DEBUG)INSTRUMENT_SIZE: %d\n", ftm_data->instrument.size());
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

    int8_t sample_loop = false;

    size_t sample_len = 0;

    int8_t sample_var = 0;

    int tick_length = SAMP_RATE / ENG_SPEED;
    std::vector<int16_t> tick_buf;

    WAVE_TYPE mode;

    //FX Flag
    uint8_t slide_up = 0;
    uint8_t slide_down = 0;

    uint8_t vol_slide_up = 0;
    uint8_t vol_slide_down = 0;

    uint8_t vol_count = 0;

public:

    void init(FTM_FILE* data) {
        ftm_data = data;
        inst_proc.init(data);
        tick_buf.resize(tick_length);
        printf("INIT: ftm_data = %p, tick_length = (%d / %d) = %d\n", ftm_data, SAMP_RATE, ENG_SPEED, tick_length);
    }

    void set_slide_up(uint8_t n) {
        slide_up = n;
        slide_down = 0;
    }

    void set_slide_down(uint8_t n) {
        slide_down = n;
        slide_up = 0;
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

    void make_tick_sound() {
        if (mode < 5) {
            int8_t env_vol = inst_proc.get_sequ_var(VOL_SEQU);
            if (mode == TRIANGULAR) env_vol = env_vol ? 15 : 0;
            // printf("(DEBUG: VOL) GET_ENV: %d->%d\n", inst_proc.get_pos(VOL_SEQU), inst_proc.get_sequ_var(VOL_SEQU));
            for (int i = 0; i < tick_length; i++) {
                i_pos = i_pos & 31;
                tick_buf[i] = wave_table[mode][i_pos] * env_vol * chl_vol;
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
            for (int i = 0; i < tick_length; i++) {
                tick_buf[i] = nes_noise_make(mode - 6) * env_vol * chl_vol;
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
                                    sample_pos = 0;
                                    sample_status = false;
                                }
                            }
                        }
                    }
                    tick_buf[i] = sample_var * 64;
                }
            }
        } else {
            memset(tick_buf.data(), 0, tick_buf.size() * sizeof(int16_t));
        }
    }

    void update_tick() {
        if (inst_proc.get_sequ_enable(PIT_SEQU) || inst_proc.get_sequ_enable(HPI_SEQU)) {
            if (mode < 5) {
                set_period(get_period()
                            + (float)inst_proc.get_sequ_var(PIT_SEQU) +
                            (float)(inst_proc.get_sequ_var(HPI_SEQU) * 16));
            } else if (mode > 5) {
                // printf("PIT: %d->%d\n", inst_proc.get_pos(PIT_SEQU), inst_proc.get_sequ_var(PIT_SEQU));
                set_noise_rate((noise_rate_rel - inst_proc.get_sequ_var(PIT_SEQU)) & 15);
            }
        }

        if (inst_proc.get_sequ_enable(ARP_SEQU)) {
            if (mode < 5) {
                set_note_rely(base_note + inst_proc.get_sequ_var(ARP_SEQU));
            } else if (mode > 5) {
                // printf("ARP: %d->%d\n", inst_proc.get_pos(ARP_SEQU), inst_proc.get_sequ_var(ARP_SEQU));
                set_noise_rate_rel((noise_rate + inst_proc.get_sequ_var(ARP_SEQU)) & 15);
            }
        }

        if (inst_proc.get_sequ_enable(DTY_SEQU)) {
            if (mode < 4) {
                set_mode((WAVE_TYPE)inst_proc.get_sequ_var(DTY_SEQU));
            } else if (mode > 5) {
                set_mode((WAVE_TYPE)(inst_proc.get_sequ_var(DTY_SEQU) & 1));
            }
        }

        make_tick_sound();
        inst_proc.update_tick();

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
        }
    }

    void set_inst(int n) {
        // printf("SET INST: %d\n", n);
        inst_proc.set_inst(n);
    }

    void note_start() {
        if (mode == DPCM_SAMPLE) {
            sample_fpos = 0;
            sample_pos = 0;
            sample_status = true;
        } else {
            inst_proc.start();
            i_pos = rand() & 31;
        }
    }

    void note_end() {
        if (mode == DPCM_SAMPLE) {
            sample_var = 0;
            sample_status = false;
        } else {
            inst_proc.end();
        }
    }

    void set_period(float period_ref) {
        if (mode > 4) {
            set_noise_rate((uint8_t)period_ref & 15);
        } else {
            period = period_ref;
            freq = period2freq(period);
            pos_count = (freq / SAMP_RATE) * 32;
        }
        // printf("SET_PERIOD: P=%f, F=%f, C=%f\n", period, freq, pos_count);
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
            period = note2period(note, 1);
            freq = period2freq(period);
            pos_count = (freq / SAMP_RATE) * 32;
        } else if (mode == DPCM_SAMPLE) {
            sample_num = inst_proc.get_inst()->dpcm[note - 24].index - 1;
            sample_loop = inst_proc.get_inst()->dpcm[note - 24].loop;
            sample_pitch = inst_proc.get_inst()->dpcm[note - 24].pitch;
            sample_len = ftm_data->dpcm_samples[sample_num].sample_size_byte * 8;
            printf("DPCM_CHANNEL: SET NUM->%d, PITCH->%d, LOOP->%d, SMP_LEN->%d\n", sample_num, sample_pitch, sample_loop, sample_len);
            sample_pos = 0;
        }
    }

    float get_freq() {
        return freq;
    }

    float get_period() {
        return period;
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
            mode = (WAVE_TYPE)(m + NOISE0);
        } else if (mode != TRIANGULAR) {
            mode = m;
        }
        // printf("SET MODE: %d\n", mode);
    }

    WAVE_TYPE get_mode() {
        return mode;
    }

    void set_note(uint8_t note) {
        base_note = note;
        set_note_rely(note);
        // printf("SET_NOTE: P=%f, F=%f, C=%f\n", period, freq, pos_count);
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

        set_speed(ftm_data->fr_block.speed);
        set_tempo(ftm_data->fr_block.tempo);
    }

    void set_speed(int speed_ref) {
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
            buf[i] = r / 2;
        }
    }

    uint8_t get_chl_vol(int n) {
        return channel[n].get_vol();
    }

    int8_t get_chl_env_vol(int n) {
        return channel[n].get_env_vol();
    }

    void process_efx(fx_t fxdata[4], int c) {
        uint8_t count = ftm_data->ch_fx_count(c);
        for (uint8_t i = 0; i < count; i++) {
            if (fxdata[i].fx_cmd == 0x01) {
                if (fxdata[i].fx_var >= 0x20) {
                    set_tempo(fxdata[i].fx_var);
                    printf("C%d: SET TEMPO -> %d\n", c, fxdata[i].fx_var);
                } else {
                    set_speed(fxdata[i].fx_var);
                    printf("C%d: SET SPEED -> %d\n", c, fxdata[i].fx_var);
                }
            } else if (fxdata[i].fx_cmd == 0x03) {
                next_frame(fxdata[i].fx_var - 1);
                printf("C%d: SKIP TO NEXT FRAME ROW %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x12) {
                channel[c].set_mode((WAVE_TYPE)(fxdata[i].fx_var));
                printf("C%d: SET MODE -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x10) {
                channel[c].set_slide_up(fxdata[i].fx_var);
                printf("C%d: SET SLIDE_UP -> %d\n", c, fxdata[i].fx_var);
            } else if (fxdata[i].fx_cmd == 0x11) {
                channel[c].set_slide_down(fxdata[i].fx_var);
                printf("C%d: SET SLIDE_DOWN -> %d\n", c, fxdata[i].fx_var);
            }  else if (fxdata[i].fx_cmd == 0x16) {
                channel[c].set_vol_slide_up(HEX_B1(fxdata[i].fx_var));
                channel[c].set_vol_slide_down(HEX_B2(fxdata[i].fx_var));
                printf("C%d: SET VOL_SLIDE, UP->%d - DOWN->%d = %d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var), HEX_B1(fxdata[i].fx_var) - HEX_B2(fxdata[i].fx_var));
            } else if (fxdata[i].fx_cmd != NO_EFX) {
                printf("UNKNOW CMD: %02X, PARAM=%02X\n", fxdata[i].fx_cmd, fxdata[i].fx_var);
            }
        }
    }

    void process_tick() {
        tick_accumulator += 1.0f;

        while (tick_accumulator >= ticks_row) {
            for (int c = 0; c < 5; c++) {
                unpk_item_t ft_item = ftm_data->get_pt_item(c, ftm_data->frames[frame][c], row);
                if (ft_item.instrument != NO_INST) {
                    channel[c].set_inst(ft_item.instrument);
                }
                if (ft_item.note != NO_NOTE) {
                    if (ft_item.note >= 13) {
                        channel[c].note_end();
                        // printf("SET_NOTE(C%d): NOTE_CUT\n", c);
                    } else {
                        // printf("SET_NOTE(C%d): %f\n", c, channel[c].get_freq());
                        channel[c].set_note(item2note(ft_item.note, ft_item.octave));
                        channel[c].note_start();
                        // channel[c].set_vol(15);
                    }
                }
                if (ft_item.volume != NO_VOL) {
                    channel[c].set_vol(ft_item.volume);
                    // printf("SET_VOL(C%d): %d\n", c, channel[c].get_vol());
                }
                process_efx(ft_item.fxdata, c);
            }

            row++;
            if (row >= ftm_data->fr_block.pat_length) {
                next_frame(0);
            }

            tick_accumulator -= ticks_row;
        }

        mix_all_channel();
    }

    void next_frame(int r) {
        row = r;
        frame++;
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

private:
    void recalculate_ticks_row() {
        ticks_row = (float)(150 * speed) / (float)tempo;
    }
};

#endif // TRACKER_H