#ifndef FTM_FILE_H
#define FTM_FILE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>

#include "fami32_pin.h"

extern "C" {
#include "dpcm.h"
}

#define FILE_OPEN_ERROR -1
#define FTM_NOT_VALID -2
#define NO_SUPPORT_VERSION -3
#define NO_SUPPORT_EXTCHIP -4
#define NO_SUPPORT_MULTITRACK -5

#define FAMI32_BASE_CHANNELS 5
#define FAMI32_VRC6_CHANNELS 3
#define FAMI32_VRC7_CHANNELS 6
#define FAMI32_VRC7_FIRST_CHANNEL 5
#define FAMI32_MMC5_CHANNELS 2
#define FAMI32_FDS_CHANNELS 1
#define FAMI32_N163_MAX_CHANNELS 8
#define FAMI32_MAX_CHANNELS 25

#define FTM_EXT_NONE 0x00
#define FTM_EXT_VRC6 0x01
#define FTM_EXT_VRC7 0x02
#define FTM_EXT_FDS 0x04
#define FTM_EXT_MMC5 0x08
#define FTM_EXT_N163 0x10

#define INST_NONE 0
#define INST_2A03 1
#define INST_VRC6 2
#define INST_VRC7 3
#define INST_FDS 4
#define INST_N163 5

typedef struct __attribute__((packed)) {
    char id[18] = {'F','a','m','i','T','r','a','c','k','e','r',' ','M','o','d','u','l','e'};
    uint32_t version = 0x0440;
} FTM_HEADER;

typedef struct __attribute__((packed)) {
    char id[16] = "PARAMS";
    uint32_t version = 0x00000006;
    uint32_t size = 0x0000001D;
    uint8_t ext_chip = 0x00;
    uint32_t channel = FAMI32_BASE_CHANNELS;
    uint32_t machine = 0;
    uint32_t e_speed = 0;
    uint32_t v_style = 1;
    uint32_t hl1 = 4;
    uint32_t hl2 = 16;
    uint32_t n163_channels = 1;
    uint32_t s_split = 0x20; // ?
} PARAMS_BLOCK;

typedef struct __attribute__((packed)) {
    char id[16] = "INFO";
    uint32_t version = 1;
    uint32_t size = 96;
    char title[32] = "TITLE";
    char author[32] = "AUTHOR";
    char copyright[32] = "COPYRIGHT";
} INFO_BLOCK;

typedef struct {
    uint8_t fx_cmd;
    uint8_t fx_var;
} fx_t;

typedef struct __attribute__((packed)) {
    char id[16] = "HEADER";
    uint32_t version = 3;
    uint32_t size = 20;
    uint8_t track_num = 0;
    char name[64] = "_FAMI32_";
    uint8_t ch_id[FAMI32_MAX_CHANNELS] = {0, 1, 2, 3, 4};
    uint8_t ch_fx[FAMI32_MAX_CHANNELS] = {0};
} HEADER_BLOCK;

typedef struct __attribute__((packed)) {
    char id[16] = "INSTRUMENTS";
    uint32_t version = 6;
    uint32_t size;
    uint32_t inst_num;
} INSTRUMENT_BLOCK;

typedef struct __attribute__((packed)) {
    char id[16] = "SEQUENCES";
    uint32_t version = 6;
    uint32_t size;
    uint32_t sequ_num;
} SEQUENCES_BLOCK;

typedef struct __attribute__((packed)) {
    char id[16] = "PATTERNS";
    uint32_t version = 5;
    uint32_t size;
} PATTERN_BLOCK;

typedef struct __attribute__((packed)) {
    char id[16] = "FRAMES";
    uint32_t version = 3;
    uint32_t size;
    uint32_t frame_num = 1;
    uint32_t speed = 6;
    uint32_t tempo = 150;
    uint32_t pat_length = 64;
} FRAME_BLOCK;

typedef struct __attribute__((packed)) {
    char id[16] = "DPCM SAMPLES";
    uint32_t version = 1;
    uint32_t size = 1;
    uint8_t sample_num = 0;
} DPCM_SAMPLE_BLOCK;

typedef struct {
    uint8_t index;
    uint32_t name_len;
    char name[64];
    uint32_t sample_size_byte;
    std::vector<uint8_t> dpcm_data;
    std::vector<uint8_t> pcm_data;
} dpcm_sample_t;

typedef struct __attribute__((packed)) {
    uint8_t index = 0;
    uint8_t pitch : 4 = 0;
    uint8_t loop : 4 = 0;
    uint8_t d_counte = 0xff;
} dpcm_t;

typedef struct __attribute__((packed)) {
    bool enable;
    uint8_t seq_index;
} seq_index_t;

#define SEQ_FEAT_DISABLE 0xffffffff
#define FAMI32_FDS_WAVE_SIZE 64
#define FAMI32_FDS_MOD_SIZE 32
#define FAMI32_FDS_SEQUENCE_TYPES 3
#define FAMI32_FDS_SEQUENCE_MAX 255
#define FAMI32_N163_WAVE_SIZE 32
#define FAMI32_N163_WAVE_MAX 16

typedef struct __attribute__((packed)) {
    uint8_t length = 0;
    uint32_t loop = SEQ_FEAT_DISABLE;
    uint32_t release = SEQ_FEAT_DISABLE;
    uint32_t setting = 0;
    int8_t data[FAMI32_FDS_SEQUENCE_MAX] = {0};
} fds_sequence_t;

typedef struct __attribute__((packed)) {
    uint32_t index = 0;
    uint8_t type = INST_2A03;
    uint32_t seq_count = 5;
    seq_index_t seq_index[5] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    dpcm_t dpcm[96];
    uint32_t vrc7_patch = 0;
    uint8_t vrc7_regs[8] = {0x01, 0x21, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F};
    uint8_t fds_wave[FAMI32_FDS_WAVE_SIZE] = {
        0, 1,12,22,32,36,39,39,42,47,47,50,48,51,54,58,
        54,55,49,50,52,61,63,63,59,56,53,51,48,47,41,35,
        35,35,41,47,48,51,53,56,59,63,63,61,52,50,49,55,
        54,58,54,51,48,50,47,47,42,39,39,36,32,22,12, 1
    };
    uint8_t fds_mod[FAMI32_FDS_MOD_SIZE] = {0};
    uint32_t fds_mod_speed = 0;
    uint32_t fds_mod_depth = 0;
    uint32_t fds_mod_delay = 0;
    fds_sequence_t fds_seq[FAMI32_FDS_SEQUENCE_TYPES];
    uint32_t n163_wave_size = FAMI32_N163_WAVE_SIZE;
    uint32_t n163_wave_pos = 0;
    uint32_t n163_wave_count = 1;
    uint8_t n163_wave[FAMI32_N163_WAVE_MAX][FAMI32_N163_WAVE_SIZE] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
        15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0}
    };
    uint32_t name_len = 14;
    char name[64] = "New instrument";
} instrument_t;

typedef struct {
    uint32_t index;
    uint32_t type;
    uint8_t length;
    uint32_t loop = -1;
    std::vector<int8_t> data;
    uint32_t release = -1;
    uint32_t setting = 0;
} sequences_t;

typedef struct __attribute__((packed)) {
	uint32_t row;
    uint8_t note;
	uint8_t octave;
	uint8_t instrument;
	uint8_t volume;
	fx_t fxdata[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
} ft_item_t;

#define NO_NOTE 0
#define NO_OCT 0
#define NO_INST 0x40
#define NO_VOL 0x10
#define NO_EFX 0

#define NOTE_CUT 14
#define NOTE_END 13

typedef struct {
    uint8_t note = NO_NOTE;
	uint8_t octave = NO_OCT;
	uint8_t instrument = NO_INST;
	uint8_t volume = NO_VOL;
	fx_t fxdata[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
} unpk_item_t;

typedef struct {
    uint32_t track;
    uint32_t channel;
    uint32_t index;
    uint32_t items;
    std::vector<ft_item_t> item;
} pattern_t;

const char note2str[15][3] = {
    "--", "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-", "==", "--"
};

const char fx2char[32] = {
    '.','F','B','D','C','E','3','U','H','I','0','4','7','P','G','Z','1','2','V','Y','Q','R','A','S','X','U','U','U','J','W'
};

const uint8_t fast_fx_table[16] = {
    0x0A, 0x10, 0x11, 0x06, 0x0B, 0x16, 0x02, 0x04, 0x03, 0x01, 0x0E, 0x0D, 0x14, 0x15, 0x17, 0x12
};

const uint8_t vaild_fx_table[20] = {
    0x01, 0x02, 0x03, 0x04, 0x06, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x1D
};

#define HEX_B1(n) (n >> 4)
#define HEX_B2(n) (n & 0xf)

#define SET_HEX_B1(a, b) ((a & 0x0F) | (b << 4))
#define SET_HEX_B2(a, b) ((a & 0xF0) | b)

class FTM_FILE {
private:
    std::vector<std::vector<sequences_t>> sequences;
    std::vector<std::vector<sequences_t>> vrc6_sequences;
    std::vector<std::vector<sequences_t>> n163_sequences;
    std::vector<instrument_t> instrument;

    std::vector<std::vector<uint8_t>> frames;
    std::vector<std::vector<pattern_t>> patterns;

public:
    FTM_FILE();
    void new_ftm();

    FILE *ftm_file;

    FTM_HEADER header;
    PARAMS_BLOCK pr_block;
    INFO_BLOCK nf_block;
    HEADER_BLOCK he_block;
    INSTRUMENT_BLOCK inst_block;
    SEQUENCES_BLOCK seq_block;
    SEQUENCES_BLOCK vrc6_seq_block;
    SEQUENCES_BLOCK n163_seq_block;
    FRAME_BLOCK fr_block;
    PATTERN_BLOCK pt_block;
    DPCM_SAMPLE_BLOCK dpcm_block;

    char current_file[1024];

    uint32_t sequ_max = 0;
    uint32_t pattern_num = 0;

    std::vector<std::vector<std::vector<unpk_item_t>>> unpack_pt;

    std::vector<dpcm_sample_t> dpcm_samples;

    void create_new_inst();
    void set_inst_type(int n, uint8_t type);
    void remove_inst(int n);

    int open_ftm(const char *filename);
    int save_as_ftm(const char *filename);
    int save_ftm();

    void write_file_header();

    int read_param_block();
    void write_param_block();
    void read_info_block();
    void write_info_block();

    int read_header_block();
    void write_header_block();

    void read_instrument_block();
    void write_instrument_block();
    void read_instrument_data();
    void write_instrument_data();

    void read_sequences_block();
    void read_sequences_data();
    void write_sequences();
    void read_vrc6_sequences_block();
    void read_vrc6_sequences_data();
    void write_vrc6_sequences();
    void read_n163_sequences_data();
    void write_n163_sequences();

    void read_frame_block();
    void read_frame_data();
    void write_frame();

    void read_pattern_block();
    void read_pattern_data();
    void write_pattern();

    void read_dpcm_block();
    void read_dpcm_data();
    void write_dpcm();

    unpk_item_t get_pt_item(uint8_t c, uint8_t i, uint32_t r);
    void set_pt_item(uint8_t c, uint8_t i, uint32_t r, unpk_item_t item);
    uint8_t get_frame_map(int f, int c);

    void set_frame_map(int f, int c, int n);
    void set_frame_plus1(int f, int c);
    void set_frame_minus1(int f, int c);

    void print_frame_data(int index);
    uint8_t ch_fx_count(int n);
    uint32_t get_channel_count() const;
    bool vrc7_enabled() const;
    void set_vrc7_enabled(bool enabled);
    bool is_vrc7_channel(int c) const;
    int vrc7_channel_index() const;
    bool vrc6_enabled() const;
    void set_vrc6_enabled(bool enabled);
    bool is_vrc6_channel(int c) const;
    int vrc6_channel_index() const;
    bool fds_enabled() const;
    void set_fds_enabled(bool enabled);
    bool is_fds_channel(int c) const;
    int fds_channel_index() const;
    bool mmc5_enabled() const;
    void set_mmc5_enabled(bool enabled);
    bool is_mmc5_channel(int c) const;
    int mmc5_channel_index() const;
    bool n163_enabled() const;
    void set_n163_enabled(bool enabled);
    bool is_n163_channel(int c) const;
    int n163_channel_index() const;
    uint8_t n163_channel_count() const;
    void set_n163_channel_count(uint8_t count);
    int read_ftm_all();

    int8_t get_sequ_data(int type, int index, int seq_index);
    uint8_t get_sequ_len(int type, int index);
    sequences_t* get_sequ(int type, int index);
    int8_t get_inst_sequ_data(instrument_t *inst, int type, int index, int seq_index);
    sequences_t* get_inst_sequ(instrument_t *inst, int type, int index);

    instrument_t *get_inst(int n);

    void resize_sequ_len(int type, int index, int n);
    void resize_inst_sequ_len(instrument_t *inst, int type, int index, int n);

    void resize_sequ(int type, int size);
    void resize_inst_sequ(instrument_t *inst, int type, int size);

    void set_sequ_loop(int type, int index, uint32_t n);
    uint8_t get_sequ_loop(int type, int index);
    void set_sequ_release(int type, int index, uint32_t n);
    uint8_t get_sequ_release(int type, int index);

    void new_frame();
    void insert_new_frame(int n);
    void remove_frame(int n);
    void moveup_frame(int n);
    void movedown_frame(int n);

private:
    void apply_channel_layout();
    void resize_channel_storage(uint32_t new_count);
};

extern FTM_FILE ftm;

#endif
