#ifndef TRACKER_H
#define TRACKER_H

#include <stdio.h>
#include "ftm_file.h"

typedef enum {
    NOTE_OFF,
    NOTE_ON,
    NOTE_RELEASE,
} note_stat_t;

class FAMI_INSTRUMENT {
private:
    FTM_FILE& ftm_data;
    int pos[5];
    note_stat_t status[5];
    int inst_num = 0;
    instrument_t *instrument;

public:
    FAMI_INSTRUMENT(FTM_FILE& data) : ftm_data(data) {
        printf("INIT FTM_FILE\n");
    }

    void set_inst(int n) {
        memset(pos, 0, sizeof(pos));
        inst_num = n;
        instrument = &ftm_data.instrument[n];
    }

    int get_inst_num() {
        return inst_num;
    }

    instrument_t* get_inst() {
        return instrument;
    }

    void update_tick() {
        for (int i = 0; i < 5; i++) {
            if (instrument->seq_index[i].enable) {
                sequences_t* sequ = ftm_data.sequences[instrument->seq_index[i].seq_index].data();
                pos[i]++;
                // ?
                
            } else {
                pos[i] = 0;
            };
        }
    }
};

class FAMI_CHANNEL {
public:

};

#endif // TRACKER_H