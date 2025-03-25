#include "fami32_instrument.h"

void FAMI_INSTRUMENT::init(FTM_FILE* data) {
    memset(pos, 0, sizeof(pos));
    memset(status, 0, sizeof(pos));
    ftm_data = data;
    instrument = ftm_data->get_inst(0);
    set_inst(0);
    DBG_PRINTF("INIT FTM_FILE IN %p\n", ftm_data);
}

void FAMI_INSTRUMENT::set_inst(int n) {
    memset(pos, 0, sizeof(pos));
    inst_num = n;
    instrument_t *inst_new = ftm_data->get_inst(inst_num);
    if (inst_new != NULL) instrument = inst_new;
}

int FAMI_INSTRUMENT::get_inst_num() {
    return inst_num;
}

instrument_t* FAMI_INSTRUMENT::get_inst() {
    return instrument;
}

void FAMI_INSTRUMENT::start() {
    for (int i = 0; i < 5; i++) {
        pos[i] = 0;
        if (instrument->seq_index[i].enable) {
            status[i] = SEQU_PLAYING;
            var[i] = ftm_data->get_sequ_data(i, instrument->seq_index[i].seq_index, 0);
        } else {
            status[i] = SEQU_STOP;
            var[i] = i ? 0 : 15;
        }
    }
}

void FAMI_INSTRUMENT::end() {
    for (int i = 0; i < 5; i++) {
        if (instrument->seq_index[i].enable) {
            if (status[i] != SEQU_STOP) {
                if (ftm_data->get_sequ(i, instrument->seq_index[i].seq_index)->release != SEQ_FEAT_DISABLE)
                    pos[i] = ftm_data->get_sequ(i, instrument->seq_index[i].seq_index)->release;
            }
            status[i] = SEQU_RELEASE;
        } else {
            status[i] = SEQU_STOP;
            var[i] = 0;
        }
    }
}

void FAMI_INSTRUMENT::cut() {
    memset(pos, 0, sizeof(pos));
    for (int i = 0; i < 5; i++) {
        status[i] = SEQU_STOP;
        pos[i] = 0;
        var[i] = 0;
    }
}

void FAMI_INSTRUMENT::update_tick() {
    for (int i = 0; i < 5; i++) {
        if (status[i] || (status[VOL_SEQU] == SEQU_RELEASE)) {
            sequences_t *sequ = ftm_data->get_sequ(i, instrument->seq_index[i].seq_index);
            if (sequ == NULL) {
                continue;
            }
            pos[i]++;
            if (status[i] == SEQU_PLAYING) {
                if (sequ->release != SEQ_FEAT_DISABLE) {
                    if (sequ->loop == SEQ_FEAT_DISABLE) {
                        if (pos[i] > sequ->release) {
                            pos[i]--;
                        }
                    } else {
                        if (pos[i] > sequ->loop) {
                            pos[i] = sequ->release;
                        }
                    }
                }
            } else if (status[i] == SEQU_RELEASE) {
                if (sequ->release == SEQ_FEAT_DISABLE) {
                    status[i] = SEQU_STOP;
                    if (i == 0) {
                        var[0] = 0;
                        continue;
                    }
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

int8_t FAMI_INSTRUMENT::get_sequ_var(int n) {
    return var[n];
}

uint32_t FAMI_INSTRUMENT::get_pos(int n) {
    return pos[n];
}

note_stat_t FAMI_INSTRUMENT::get_status(int n) {
    return status[n];
}

bool FAMI_INSTRUMENT::get_sequ_enable(int n) {
    return instrument->seq_index[n].enable;
}