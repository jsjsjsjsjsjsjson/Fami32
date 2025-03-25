#ifndef FAMI32_INSTRUMENT_H
#define FAMI32_INSTRUMENT_H

#include "fami32_common.h"

class FAMI_INSTRUMENT {
private:
    FTM_FILE *ftm_data;
    uint32_t pos[5];
    int8_t var[5];
    note_stat_t status[5];
    int inst_num = 0;
    instrument_t *instrument;

public:
    void init(FTM_FILE* data);

    void set_inst(int n);
    int get_inst_num();
    instrument_t* get_inst();

    void start();
    void end();
    void cut();

    void update_tick();

    int8_t get_sequ_var(int n);
    uint32_t get_pos(int n);
    note_stat_t get_status(int n);
    bool get_sequ_enable(int n);
};

#endif