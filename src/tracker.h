#ifndef TRACKER_H
#define TRACKER_H

#include <stdio.h>
#include <vector>

#include "ftm_file.h"

#include "wave_table.h"
#include "src_config.h"

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
        printf("INIT FTM_FILE\n");
    }

    void set_inst(int n) {
        memset(pos, 0, sizeof(pos));
        memset(status, 0, sizeof(pos));
        inst_num = n;
        instrument = &ftm_data->instrument[n];
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
            }
        }
    }

    void update_tick() {
        for (int i = 0; i < 5; i++) {
            if (status[i]) {
                sequences_t *sequ = ftm_data->sequences[instrument->seq_index[i].seq_index].data();
                pos[i]++;
                if ((status[i] == SEQU_RELEASE) && (pos[i] > sequ->release)) {
                    pos[i]--;
                }
                if (pos[i] >= sequ->length) {
                    if (sequ->loop == SEQ_FEAT_DISABLE) {
                        status[i] == SEQU_STOP;
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

    int tick_length = SAMP_RATE / ENG_SPEED;
    std::vector<int16_t> tick_buf;

    WAVE_TYPE mode;

public:

    void init(FTM_FILE* data) {
        ftm_data = data;
        inst_proc.init(data);
        tick_buf.resize(tick_length);
        printf("INIT: ftm_data = %p, tick_length = (%d / %d) = %d", ftm_data, SAMP_RATE, ENG_SPEED, tick_length);
    }

    void make_tick_sound() {
        int8_t env_vol = inst_proc.get_sequ_var(0);
        for (int i = 0; i < tick_length; i++) {
            tick_buf[i] = wave_table[mode][i_pos] * env_vol * chl_vol;
            f_pos += pos_count;
            if (f_pos >= 1.0f) {
                i_pos += (int)f_pos;
                f_pos -= 1.0f;
            }
        }
        inst_proc.update_tick();
    }

    void set_inst(int n) {
        inst_proc.set_inst(n);
    }

    void note_start() {
        inst_proc.start();
    }
};

#endif // TRACKER_H