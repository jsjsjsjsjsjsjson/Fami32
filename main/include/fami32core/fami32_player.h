#ifndef FAMI32_PLAYER_H
#define FAMI32_PLAYER_H

#include "fami32_common.h"
#include "fami32_channel.h"
#include "src_config.h"

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

    bool mute[5] = {false, false, false, false, false};
public:
    FAMI_CHANNEL channel[5];

    void init(FTM_FILE* data);

    void reload();

    void start_play();
    void stop_play();

    void ref_speed_and_tempo();
    void set_speed(int speed_ref);
    void set_tempo(int tempo_ref);
    int get_speed() const;
    int get_tempo() const;

    void set_ticks_row(float tr);
    float get_ticks_row() const;

    void mix_all_channel();

    uint8_t get_chl_vol(int n);
    int8_t get_chl_env_vol(int n);

    void process_delay_efx(fx_t fxdata[4], int c);
    void process_efx_pre(fx_t fxdata[4], int c);
    void process_efx_post(fx_t fxdata[4], int c);
    void process_item(unpk_item_t item, int c);
    void process_tick();

    void next_frame(int r);
    void jmp_to_frame(int f);
    int16_t* get_buf();
    size_t get_buf_size_byte() const;
    size_t get_buf_size() const;
    int get_row();
    void set_row(int r);
    int get_frame();
    uint8_t get_cur_frame_map(int c);
    bool get_play_status();
    void set_mute(int c, bool s);
    bool get_mute(int c);

private:
    void recalculate_ticks_row();
};

#endif