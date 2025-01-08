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
        printf("INIT FTM_FILE IN %p\n", ftm_data);
    }

    void set_inst(int n) {
        memset(pos, 0, sizeof(pos));
        memset(status, 0, sizeof(pos));
        inst_num = n;
        printf("(DEBUG)INSTRUMENT_SIZE: %d\n", ftm_data->instrument.size());
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

    uint8_t chl_vol = 15;

    uint8_t base_note;
    uint8_t rely_note;

    int tick_length = SAMP_RATE / ENG_SPEED;
    std::vector<int16_t> tick_buf;

    WAVE_TYPE mode;

public:

    void init(FTM_FILE* data) {
        ftm_data = data;
        inst_proc.init(data);
        tick_buf.resize(tick_length);
        printf("INIT: ftm_data = %p, tick_length = (%d / %d) = %d\n", ftm_data, SAMP_RATE, ENG_SPEED, tick_length);
    }

    void make_tick_sound() {
        if (mode < 5) {
            int8_t env_vol = 15;
            if (mode != 2) env_vol = inst_proc.get_sequ_var(VOL_SEQU);
            for (int i = 0; i < tick_length; i++) {
                i_pos = i_pos & 31;
                tick_buf[i] = wave_table[mode][i_pos] * env_vol * chl_vol;
                f_pos += pos_count;
                if (f_pos > 1.0f) {
                    i_pos += (int)f_pos;
                    f_pos -= (int)f_pos;
                }
            }
        }
    }

    void update_tick() {
        inst_proc.update_tick();
        // printf("(DEBUG)PIT: GET(%f) + ", get_period());
        set_period(get_period()
                    + (float)inst_proc.get_sequ_var(PIT_SEQU) +
                    (float)(inst_proc.get_sequ_var(HPI_SEQU) * 10));
        
        // printf("PIT(%d) + HPI(%d)*10 = %f\n", inst_proc.get_sequ_var(PIT_SEQU), (inst_proc.get_sequ_var(HPI_SEQU) * 10), get_period());

        if (!inst_proc.get_sequ_var(PIT_SEQU)
                && !inst_proc.get_sequ_var(HPI_SEQU)) set_note_rely(base_note + inst_proc.get_sequ_var(ARP_SEQU));
        if (mode < 4 && inst_proc.get_status(4) == SEQU_STOP) {
            set_mode((WAVE_TYPE)inst_proc.get_sequ_var(DTY_SEQU));
        }
        make_tick_sound();
    }

    void set_inst(int n) {
        printf("SET INST: %d\n", n);
        inst_proc.set_inst(n);
    }

    void note_start() {
        inst_proc.start();
    }

    void note_end() {
        inst_proc.end();
    }

    void set_period(float period_ref) {
        period = period_ref;
        freq = period2freq(period);
        pos_count = (freq / SAMP_RATE) * 32;
        // printf("SET_PERIOD: P=%f, F=%f, C=%f\n", period, freq, pos_count);
    }

    void set_freq(float freq_ref) {
        freq = freq_ref;
        period = freq2period(freq);
        pos_count = (freq / SAMP_RATE) * 32;
    }

    void set_note_rely(uint8_t note) {
        rely_note = note;
        period = note2period(note, 0);
        freq = period2freq(period);
        pos_count = (freq / SAMP_RATE) * 32;
    }

    float get_freq() {
        return freq;
    }

    float get_period() {
        return period;
    }

    void set_mode(WAVE_TYPE m) {
        mode = m;
        printf("SET MODE: %d\n", mode);
    }

    void set_note(uint8_t note) {
        base_note = note;
        set_note_rely(note);
        // printf("SET_NOTE: P=%f, F=%f, C=%f\n", period, freq, pos_count);
    }

    void set_vol(uint8_t vol) {
        chl_vol = vol;
    }

    uint8_t get_vol() {
        return chl_vol;
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
    FAMI_CHANNEL channel[4];
    uint32_t row = 0;

    int tick_count = 0;

    int speed = 150;
    int tempo = 3;

    int frame = 0;

    int ticks_row = 0;

    std::vector<int16_t> buf;

public:
    void init(FTM_FILE* data) {
        ftm_data = data;
        channel[0].init(ftm_data);
        channel[1].init(ftm_data);

        channel[2].init(ftm_data);
        channel[2].set_mode(TRIANGULAR);

        channel[3].init(ftm_data);
        channel[4].set_mode(NOISE);

        buf.resize(channel[0].get_buf_size());

        set_speed(ftm_data->fr_block.speed);
        set_tempo(ftm_data->fr_block.tempo);
    }

    void set_speed(int speed_ref) {
        speed = speed_ref;
        ticks_row = 150 * speed / tempo;
    }

    void set_tempo(int tempo_ref) {
        tempo = tempo_ref;
        ticks_row = 150 * speed / tempo;
    }

    int get_speed() {
        return speed;
    }

    int get_tempo() {
        return tempo;
    }

    void set_ticks_row(int tr) {
        ticks_row = tr;
    }

    int get_ticks_row() {
        return ticks_row;
    }

    void mix_all_channel() {
        for (int c = 0; c < 4; c++) {
            channel[c].update_tick();
        }
        for (size_t i = 0; i < buf.size(); i++) {
            int16_t r = 0;
            for (int c = 0; c < 1; c++) {
                r += channel[c].get_buf()[i];
            }
            buf[i] = r * 2;
        }
    }

    void process_tick() {
        tick_count++;
        if (tick_count > ticks_row) {
            for (int c = 0; c < 4; c++) {
                unpk_item_t ft_item = ftm_data->get_pt_item(c, ftm_data->frames[frame][c], row);
                if (ft_item.instrument != NO_INST) {
                    channel[c].set_inst(ft_item.instrument);
                }
                if (ft_item.note != NO_NOTE) {
                    if (ft_item.note >= 13) {
                        channel[c].note_end();
                    } else {
                        channel[c].set_note(item2note(ft_item.note, ft_item.octave));
                        channel[c].note_start();
                        channel[c].set_vol(15);
                    }
                }
                if (ft_item.volume != NO_VOL) {
                    channel[c].set_vol(ft_item.volume);
                }
                if ((ft_item.fxdata[0].fx_cmd + ft_item.fxdata[0].fx_var) != NO_EFX) {
                    if (ft_item.fxdata[0].fx_cmd == 0x12) {
                        printf("0x12: %d\n", ft_item.fxdata[0].fx_var);
                        channel[c].set_mode((WAVE_TYPE)ft_item.fxdata[0].fx_var);
                    }//  else if (ft_item.fxdata[0].fx_cmd == 0x01) {
                    //    set_speed(ft_item.fxdata[0].fx_var);
                    //}
                }
            }
            row++;
            if (row >= ftm_data->fr_block.pat_length) {
                row = 0;
                frame++;
            }
            tick_count = 0;
        }
        mix_all_channel();
    }

    int16_t* get_buf() {
        return buf.data();
    }

    size_t get_buf_size_byte() {
        return buf.size() * sizeof(int16_t);
    }

    size_t get_buf_size() {
        return buf.size();
    }
};

#endif // TRACKER_H