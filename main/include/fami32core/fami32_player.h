#ifndef FAMI32_PLAYER_H
#define FAMI32_PLAYER_H

#include "fami32_common.h"
#include "fami32_channel.h"
#include "vrc7_synth.h"
#include "src_config.h"

typedef bool (*fami_timeline_next_event_cb)(uint64_t window_start_sample,
                                            uint64_t window_end_sample,
                                            uint64_t *event_sample,
                                            void *user);
typedef void (*fami_timeline_dispatch_cb)(uint64_t sample_time, void *user);

class FAMI_PLAYER {
private:
    FTM_FILE *ftm_data;

    int row = 0;
    int frame = 0;

    float tick_accumulator = 0.0f;

    int speed = 150;
    int tempo = 3;

    float ticks_row = 0.0f;
    uint32_t row_event_counter = 0;
    uint64_t audio_sample_clock = 0;

    std::vector<int16_t> buf;

    uint8_t delay_count[FAMI32_MAX_CHANNELS] = {0};
    bool delay_status[FAMI32_MAX_CHANNELS] = {0};

    unpk_item_t ft_items[FAMI32_MAX_CHANNELS];

    LowPassFilter lpf;
    HighPassFilter hpf;

    bool play_status = false;

    bool mute[FAMI32_MAX_CHANNELS] = {false};
    VRC7_SYNTH vrc7;

    bool defer_row_navigation = false;
    bool pending_jump_frame = false;
    bool pending_next_frame = false;
    int pending_jump_frame_target = 0;
    int pending_next_frame_row = 0;
public:
    FAMI_CHANNEL channel[FAMI32_MAX_CHANNELS];

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
    void process_tick(fami_timeline_next_event_cb next_event,
                      fami_timeline_dispatch_cb dispatch_event,
                      void *timeline_user);
    void reset_audio_sample_clock();
    uint64_t get_audio_sample_clock() const;

    void next_frame(int r);
    void jmp_to_frame(int f);
    int16_t* get_buf();
    size_t get_buf_size_byte() const;
    size_t get_buf_size() const;
    int get_row();
    void set_row(int r);
    int get_frame();
    uint32_t get_row_event_counter() const;
    uint8_t get_cur_frame_map(int c);
    bool get_play_status();
    void set_mute(int c, bool s);
    bool get_mute(int c);
    uint32_t get_channel_count() const;

private:
    void process_tick_events();
    void begin_channel_tick();
    void end_channel_tick();
    void render_tick_segment(size_t dst_offset, size_t sample_count, bool advance_tick_phase);
    void recalculate_ticks_row();
    void setup_channel_modes();
    void sync_vrc7_registers();
    void request_jmp_to_frame(int f);
    void request_next_frame(int r);
    void finish_row_navigation();
};

#endif
