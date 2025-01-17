#ifndef FTM_FILE_H
#define FTM_FILE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <vector>

extern "C" {
#include "dpcm.h"
}

typedef struct __attribute__((packed)) {
    char id[18];
    uint16_t version = 0x0440;
    uint16_t recv = 0;
} FTM_HEADER;

typedef struct __attribute__((packed)) {
    char id[16] = "PARAMS";
    uint32_t version = 0x00000006;
    uint32_t size = 0x0000001D;
    uint8_t ext_chip = 0x00;
    uint32_t channel = 0x00000005;
    uint32_t machine = 0;
    uint32_t e_speed = 0;
    uint32_t v_style = 1;
    uint32_t hl1 = 4;
    uint32_t hl2 = 16;
    uint32_t s_split = 0x15; // ?
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
    uint32_t size;
    uint8_t track_num = 1;
    char name[64];
    uint8_t ch_id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t ch_fx[8] = {0, 0, 0, 0, 0, 0, 0, 0};
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
    uint32_t size;
    uint16_t sample_num = 0;
} DPCM_SAMPLE_BLOCK;

typedef struct {
    uint32_t name_len;
    char name[64];
    uint32_t sample_size_byte;
    std::vector<uint8_t> dpcm_data;
    std::vector<int8_t> pcm_data;
} dpcm_sample_t;

typedef struct __attribute__((packed)) {
    uint8_t index;
    uint8_t pitch : 4;
    uint8_t loop : 4;
    uint8_t d_counte;
} dpcm_t;

typedef struct {
    bool enable;
    uint8_t seq_index;
} seq_index_t;

typedef struct __attribute__((packed)) {
    uint32_t index = 0;
    uint8_t type = 0;
    uint32_t seq_count = 5;
    seq_index_t seq_index[5] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
    dpcm_t dpcm[96];
    uint32_t name_len = 14;
    char name[64] = "New Instrument";
} instrument_t;

typedef struct __attribute__((packed)) {
    uint32_t index;
    uint32_t type;
    uint8_t length;
    uint32_t loop;
    std::vector<int8_t> data;
    uint32_t release;
    uint32_t setting;
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

#define SEQ_FEAT_DISABLE 0xffffffff

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
    '.',
    'F',
    'B',
    'D',
    'C',
    'E',
    '3',
    'U',
    'H',
    'I',
    '0',
    '4',
    '7',
    'P',
    'G',
    'Z',
    '1',
    '2',
    'V',
    'Y',
    'Q',
    'R',
    'A',
    'S',
    'X',
    'U',
    'U',
    'U',
    'J',
    'W'
};

#define HEX_B1(n) (n >> 4)
#define HEX_B2(n) (n & 0xf)

#define SET_HEX_B1(a, b) ((a & 0x0F) | (b << 4))
#define SET_HEX_B2(a, b) ((a & 0xF0) | b)

class FTM_FILE {
private:
    std::vector<std::vector<sequences_t>> sequences;
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
    FRAME_BLOCK fr_block;
    PATTERN_BLOCK pt_block;
    DPCM_SAMPLE_BLOCK dpcm_block;

    uint32_t sequ_max = 0;
    uint32_t pattern_num = 0;

    std::vector<std::vector<std::vector<unpk_item_t>>> unpack_pt;

    std::vector<dpcm_sample_t> dpcm_samples;

    void create_new_inst();

    int open_ftm(const char *filename);

    void read_param_block();
    void read_info_block();

    void read_header_block();

    void read_instrument_block();
    void read_instrument_data();

    void read_sequences_block();
    void read_sequences_data();

    void read_frame_block();
    void read_frame_data();

    void read_pattern_block();
    void read_pattern_data();

    void read_dpcm_block();
    void read_dpcm_data();

    unpk_item_t get_pt_item(uint8_t c, uint8_t i, uint32_t r);
    void set_pt_item(uint8_t c, uint8_t i, uint32_t r, unpk_item_t item);
    uint8_t get_frame_map(int f, int c);

    void set_frame_map(int f, int c, int n);
    void set_frame_plus1(int f, int c);
    void set_frame_minus1(int f, int c);

    void print_frame_data(int index);
    uint8_t ch_fx_count(int n);
    void read_ftm_all();

    int8_t get_sequ_data(int type, int index, int seq_index);
    uint8_t get_sequ_len(int type, int index);
    sequences_t* get_sequ(int type, int index);

    instrument_t *get_inst(int n);

    void resize_sequ(int type, int index, int n);

    void set_sequ_loop(int type, int index, uint32_t n);
    uint8_t get_sequ_loop(int type, int index);
    void set_sequ_release(int type, int index, uint32_t n);
    uint8_t get_sequ_release(int type, int index);

    void new_frame() {
        uint8_t next_pt = frames.size();
        std::vector<uint8_t> next_fr{next_pt, next_pt, next_pt, next_pt, next_pt};
        frames.push_back(next_fr);
        fr_block.frame_num = frames.size();
    }

    void insert_new_frame(int n) {
        uint8_t next_pt = frames.size();
        std::vector<uint8_t> next_fr{next_pt, next_pt, next_pt, next_pt, next_pt};
        frames.insert(frames.begin() + n, next_fr);
        fr_block.frame_num = frames.size();
    }

    void remove_frame(int n) {
        frames.erase(frames.begin() + n);
        fr_block.frame_num = frames.size();
    }

    void moveup_frame(int n) {
        if (n > 0 && n < frames.size()) {
            std::swap(frames[n], frames[n - 1]);
        }
    }

    void movedown_frame(int n) {
        if (n < frames.size() - 1) {
            std::swap(frames[n], frames[n + 1]);
        }
    }
};

extern FTM_FILE ftm;

#endif