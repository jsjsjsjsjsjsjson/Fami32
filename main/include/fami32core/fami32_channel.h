#ifndef FAMI32_CHANNEL_H
#define FAMI32_CHANNEL_H

#include "fami32_common.h"
#include "fami32_instrument.h"

#define FIR_TAPS 8
#define FIR_COEF_SHIFT 12

class FAMI_CHANNEL {
private:
    FTM_FILE *ftm_data;
    FAMI_INSTRUMENT inst_proc;

    int i_pos;
    float f_pos;

    float pos_count;

    float freq = 0;
    float period = 0;

    int8_t chl_vol = 15;

    uint8_t last_note = 255;

    uint8_t noise_rate = 0;
    uint8_t noise_rate_rel = 0;
    uint16_t noise_shift_reg = 1;
    float noise_timer = 0.0f;
    float noise_period = 4.0f;
    uint8_t triangle_hold_level = 8;

    int sample_pos = 0;
    float sample_fpos = 0.0f;
    uint8_t sample_pitch = 0;
    int sample_num = 0;
    bool sample_status = false;

    uint8_t sample_start = 0;

    int8_t sample_loop = false;

    size_t sample_len = 0;

    uint8_t sample_var = 0;
    uint8_t dmc_hold_level = 0;

    int tick_length;
    std::vector<int16_t> tick_buf;
    std::vector<uint8_t> apu_level_buf;

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

    uint8_t tre_spd = 0;
    uint8_t tre_dep = 0;

    uint8_t tre_pos = 0;

    float portup_target;
    uint8_t portup_speed = 0;

    float portdown_target;
    uint8_t portdown_speed = 0;

    int8_t period_offset = 0;

    float period_rely;

    uint8_t delay_cut_count = 0;
    bool delay_cut_status = false;

    uint8_t arp_fx_n1;
    uint8_t arp_fx_n2;

    uint8_t arp_fx_pos = 0;

    uint8_t rel_vol;

    uint8_t vrc7_channel_index = 0;
    uint8_t vrc7_patch = 0;
    uint8_t vrc7_regs[8] = {0x01, 0x21, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F};
    bool vrc7_gate = false;
    bool vrc7_release = false;
    bool vrc7_trigger = false;
    uint16_t vrc7_fnum = 0;
    uint8_t vrc7_block = 0;

    float fds_pos = 0.0f;
    uint8_t fds_wave[FAMI32_FDS_WAVE_SIZE] = {0};
    uint8_t fds_mod[FAMI32_FDS_MOD_SIZE] = {0};
    uint32_t fds_mod_speed = 0;
    uint32_t fds_mod_depth = 0;
    uint32_t fds_mod_delay = 0;
    uint32_t fds_mod_counter = 0;
    bool fds_gate = false;

    HighPassFilter hpf;

    struct FirState {
        int32_t hist[FIR_TAPS];
        uint8_t wr;   // ring write index
    } pcm_fir_state;
    FirState noise_level_fir_state;

public:
    uint8_t base_note;
    uint8_t rely_note;

    void clear_all_fx_flag();

    void init(FTM_FILE* data);

    void set_slide_up(uint8_t n);
    void set_slide_down(uint8_t n);
    void set_vol_slide_up(uint8_t n);
    void set_vol_slide_down(uint8_t n);
    void set_auto_port(uint8_t n);
    void set_vibrato(uint8_t spd, uint8_t dep);
    void set_tremolo(uint8_t spd, uint8_t dep);
    void set_period_offset(int8_t var);
    void set_port_up(uint8_t spd, uint8_t offset);
    void set_port_down(uint8_t spd, uint8_t offset);
    void set_delay_cut(uint8_t t);

    void make_tick_sound();
    void update_tick();

    void set_inst(int n);
    instrument_t *get_inst();

    void set_dpcm_offset(uint8_t n);
    void set_dpcm_var(uint8_t n);
    void set_dpcm_pitch(uint8_t n);
    void set_arp_fx(uint8_t n1, uint8_t n2);

    void note_start();
    void note_end();
    void note_cut();

    void set_period(float period_ref);
    void set_freq(float freq_ref);
    void set_note_rely(uint8_t note);

    float get_freq();
    float get_period();
    float get_period_rel();
    void set_noise_rate(uint8_t rate);
    void set_noise_rate_rel(uint8_t rate);
    uint8_t get_noise_rate();
    uint8_t get_dpcm_pitch();
    int get_dpcm_sample_num();
    uint8_t get_dpcm_sample_start();
    bool get_dpcm_sample_loop();
    bool get_dpcm_sample_status();

    void set_mode(WAVE_TYPE m);
    void set_chl_mode(WAVE_TYPE m);
    WAVE_TYPE get_mode();

    void set_vrc7_channel(uint8_t n);
    uint8_t get_vrc7_channel() const;
    bool is_vrc7_mode() const;
    bool get_vrc7_gate() const;
    bool get_vrc7_release() const;
    bool consume_vrc7_trigger();
    uint8_t get_vrc7_patch() const;
    uint8_t get_vrc7_reg(uint8_t reg) const;
    uint16_t get_vrc7_fnum() const;
    uint8_t get_vrc7_block() const;
    uint8_t get_vrc7_volume() const;

    bool is_fds_mode() const;
    bool get_fds_gate() const;
    void set_fds_mod_depth(uint8_t depth);
    void set_fds_mod_speed_hi(uint8_t value);
    void set_fds_mod_speed_lo(uint8_t value);

    void set_note(uint8_t note);
    void set_vol(int8_t vol);

    int8_t get_vol();
    int8_t get_env_vol();
    uint8_t get_rel_vol();
    int16_t* get_buf();
    size_t get_buf_size_byte();
    size_t get_buf_size();
    uint8_t* get_apu_level_buf();
    size_t get_apu_level_buf_size_byte();
    int get_samp_pos();
    size_t get_samp_len();
    uint32_t get_inst_pos(int sequ_type);

private:
    void reset_fir_state();
    void reset_noise_state();
    void update_noise_period();
    bool next_noise_bit(bool short_mode, float step);
    void sync_vrc7_instrument();
    void sync_fds_instrument();
    void fir_push(FirState &state, int32_t x);
    int32_t fir_apply(const FirState &state) const;
};

#endif
