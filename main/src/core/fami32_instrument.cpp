#include "fami32_instrument.h"

namespace {

bool is_fds_sequence_type(int n) {
    return n >= 0 && n < FAMI32_FDS_SEQUENCE_TYPES;
}

bool current_sequence_enable(instrument_t *instrument, int n) {
    if (instrument == NULL) {
        return false;
    }
    if (instrument->type == INST_FDS) {
        return is_fds_sequence_type(n) && instrument->fds_seq[n].length > 0;
    }
    return n >= 0 && n < 5 && instrument->seq_index[n].enable;
}

uint8_t current_sequence_length(instrument_t *instrument, FTM_FILE *ftm_data, int n) {
    if (instrument == NULL) {
        return 0;
    }
    if (instrument->type == INST_FDS) {
        return is_fds_sequence_type(n) ? instrument->fds_seq[n].length : 0;
    }
    sequences_t *seq = ftm_data->get_sequ(n, instrument->seq_index[n].seq_index);
    return seq != NULL ? seq->length : 0;
}

uint32_t current_sequence_loop(instrument_t *instrument, FTM_FILE *ftm_data, int n) {
    if (instrument == NULL) {
        return SEQ_FEAT_DISABLE;
    }
    if (instrument->type == INST_FDS) {
        return is_fds_sequence_type(n) ? instrument->fds_seq[n].loop : SEQ_FEAT_DISABLE;
    }
    sequences_t *seq = ftm_data->get_sequ(n, instrument->seq_index[n].seq_index);
    return seq != NULL ? seq->loop : SEQ_FEAT_DISABLE;
}

uint32_t current_sequence_release(instrument_t *instrument, FTM_FILE *ftm_data, int n) {
    if (instrument == NULL) {
        return SEQ_FEAT_DISABLE;
    }
    if (instrument->type == INST_FDS) {
        return is_fds_sequence_type(n) ? instrument->fds_seq[n].release : SEQ_FEAT_DISABLE;
    }
    sequences_t *seq = ftm_data->get_sequ(n, instrument->seq_index[n].seq_index);
    return seq != NULL ? seq->release : SEQ_FEAT_DISABLE;
}

int8_t current_sequence_data(instrument_t *instrument, FTM_FILE *ftm_data, int n, uint32_t index) {
    if (instrument == NULL) {
        return 0;
    }
    if (instrument->type == INST_FDS) {
        if (!is_fds_sequence_type(n) || index >= instrument->fds_seq[n].length) {
            return 0;
        }
        return instrument->fds_seq[n].data[index];
    }
    return ftm_data->get_sequ_data(n, instrument->seq_index[n].seq_index, index);
}

int8_t default_sequence_value(int n, instrument_t *instrument) {
    if (n == VOL_SEQU) {
        return (instrument != NULL && instrument->type == INST_FDS) ? 31 : 15;
    }
    return 0;
}

} // namespace

void FAMI_INSTRUMENT::init(FTM_FILE* data) {
    memset(pos, 0, sizeof(pos));
    memset(status, 0, sizeof(status));
    memset(var, 0, sizeof(var));
    ftm_data = data;
    instrument = ftm_data->get_inst(0);
    set_inst(0);
    DBG_PRINTF("INIT FTM_FILE IN %p\n", ftm_data);
}

void FAMI_INSTRUMENT::set_inst(int n) {
    memset(pos, 0, sizeof(pos));
    memset(status, 0, sizeof(status));
    memset(var, 0, sizeof(var));
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
        if (current_sequence_enable(instrument, i)) {
            status[i] = SEQU_PLAYING;
            var[i] = current_sequence_data(instrument, ftm_data, i, 0);
        } else {
            status[i] = SEQU_STOP;
            var[i] = default_sequence_value(i, instrument);
        }
    }
}

void FAMI_INSTRUMENT::end() {
    for (int i = 0; i < 5; i++) {
        if (current_sequence_enable(instrument, i)) {
            if (status[i] != SEQU_STOP) {
                uint32_t release = current_sequence_release(instrument, ftm_data, i);
                if (release != SEQ_FEAT_DISABLE) {
                    pos[i] = release;
                }
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
        if (status[i] != SEQU_PLAYING && status[i] != SEQU_RELEASE) {
            continue;
        }

        uint8_t length = current_sequence_length(instrument, ftm_data, i);
        if (length == 0) {
            continue;
        }

        uint32_t loop = current_sequence_loop(instrument, ftm_data, i);
        uint32_t release = current_sequence_release(instrument, ftm_data, i);

        pos[i]++;

        if (status[i] == SEQU_PLAYING) {
            if (release != SEQ_FEAT_DISABLE) {
                if (loop == SEQ_FEAT_DISABLE) {
                    if (pos[i] > release) {
                        pos[i] = release;
                    }
                } else {
                    if (pos[i] > release) {
                        pos[i] = loop;
                    }
                }
            }
        } else if (status[i] == SEQU_RELEASE) {
            if (release == SEQ_FEAT_DISABLE) {
                status[i] = SEQU_STOP;
                if (i == VOL_SEQU) {
                    var[i] = 0;
                }
                continue;
            }
        }

        if (pos[i] >= length) {
            if (loop == SEQ_FEAT_DISABLE) {
                pos[i] = length - 1;
                status[i] = SEQU_STOP;
            } else {
                pos[i] = loop;
                if (pos[i] >= length) {
                    pos[i] = length - 1;
                }
            }
        }

        var[i] = current_sequence_data(instrument, ftm_data, i, pos[i]);
    }
}

int8_t FAMI_INSTRUMENT::get_sequ_var(int n) {
    if (n < 0 || n >= 5) {
        return 0;
    }
    return var[n];
}

uint32_t FAMI_INSTRUMENT::get_pos(int n) {
    if (n < 0 || n >= 5) {
        return 0;
    }
    return pos[n];
}

note_stat_t FAMI_INSTRUMENT::get_status(int n) {
    if (n < 0 || n >= 5) {
        return SEQU_STOP;
    }
    return status[n];
}

bool FAMI_INSTRUMENT::get_sequ_enable(int n) {
    return current_sequence_enable(instrument, n);
}
