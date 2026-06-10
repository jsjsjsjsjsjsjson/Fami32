#include "ftm_file.h"

extern int errno;
extern bool _debug_print;

namespace {

constexpr uint32_t FTM_SEQUENCE_TYPES = 5;
constexpr uint32_t FTM_DPCM_NOTES = 96;
constexpr uint32_t FTM_MAX_DPCM_SAMPLE_SIZE = 0x0FF1;
constexpr uint8_t FTM_CH_ID_VRC6_PULSE1 = 5;
constexpr uint8_t FTM_CH_ID_FDS = 19;
constexpr uint8_t FTM_CH_ID_MMC5_SQUARE1 = 8;
constexpr uint8_t FTM_CH_ID_N163_CH1 = 11;

using SequenceBank = std::vector<std::vector<sequences_t>>;

size_t bounded_strlen(const char *str, size_t max_len) {
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        ++len;
    }
    return len;
}

void write_u8(FILE *file, uint8_t value) {
    fwrite(&value, sizeof(value), 1, file);
}

void write_u32(FILE *file, uint32_t value) {
    fwrite(&value, sizeof(value), 1, file);
}

bool read_u8(FILE *file, uint8_t *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}

bool read_u32(FILE *file, uint32_t *value) {
    return fread(value, sizeof(*value), 1, file) == 1;
}

bool is_valid_instrument_slot(const std::vector<instrument_t> &instruments, size_t index) {
    return index < instruments.size() && instruments[index].index == index;
}

uint32_t instrument_name_length(instrument_t &inst) {
    inst.name_len = static_cast<uint32_t>(bounded_strlen(inst.name, sizeof(inst.name)));
    return inst.name_len;
}

uint8_t make_dpcm_pitch_byte(const dpcm_t &assignment) {
    return static_cast<uint8_t>((assignment.pitch & 0x0F) | (assignment.loop ? 0x80 : 0x00));
}

uint32_t normalized_dpcm_size(uint32_t size) {
    if (size == 0) {
        return 0;
    }
    uint32_t padded = size + ((1U - size) & 0x0FU);
    if (padded > FTM_MAX_DPCM_SAMPLE_SIZE) {
        return FTM_MAX_DPCM_SAMPLE_SIZE;
    }
    return padded;
}

uint8_t clamp_n163_channels(uint32_t count) {
    if (count < 1) {
        return 1;
    }
    if (count > FAMI32_N163_MAX_CHANNELS) {
        return FAMI32_N163_MAX_CHANNELS;
    }
    return (uint8_t)count;
}

uint32_t channel_count_for_ext(uint8_t ext_chip, uint8_t n163_channels) {
    uint32_t channels = FAMI32_BASE_CHANNELS;
    if (ext_chip & FTM_EXT_VRC6) {
        channels += FAMI32_VRC6_CHANNELS;
    }
    if (ext_chip & FTM_EXT_VRC7) {
        channels += FAMI32_VRC7_CHANNELS;
    }
    if (ext_chip & FTM_EXT_MMC5) {
        channels += FAMI32_MMC5_CHANNELS;
    }
    if (ext_chip & FTM_EXT_FDS) {
        channels += FAMI32_FDS_CHANNELS;
    }
    if (ext_chip & FTM_EXT_N163) {
        channels += clamp_n163_channels(n163_channels);
    }
    return channels;
}

int vrc7_channel_index_for_ext(uint8_t ext_chip) {
    if ((ext_chip & FTM_EXT_VRC7) == 0) {
        return -1;
    }
    return FAMI32_BASE_CHANNELS + ((ext_chip & FTM_EXT_VRC6) ? FAMI32_VRC6_CHANNELS : 0);
}

int vrc6_channel_index_for_ext(uint8_t ext_chip) {
    if ((ext_chip & FTM_EXT_VRC6) == 0) {
        return -1;
    }
    return FAMI32_BASE_CHANNELS;
}

int mmc5_channel_index_for_ext(uint8_t ext_chip) {
    if ((ext_chip & FTM_EXT_MMC5) == 0) {
        return -1;
    }
    int index = FAMI32_BASE_CHANNELS;
    if (ext_chip & FTM_EXT_VRC6) {
        index += FAMI32_VRC6_CHANNELS;
    }
    if (ext_chip & FTM_EXT_VRC7) {
        index += FAMI32_VRC7_CHANNELS;
    }
    return index;
}

int fds_channel_index_for_ext(uint8_t ext_chip) {
    if ((ext_chip & FTM_EXT_FDS) == 0) {
        return -1;
    }
    int index = FAMI32_BASE_CHANNELS;
    if (ext_chip & FTM_EXT_VRC6) {
        index += FAMI32_VRC6_CHANNELS;
    }
    if (ext_chip & FTM_EXT_VRC7) {
        index += FAMI32_VRC7_CHANNELS;
    }
    if (ext_chip & FTM_EXT_MMC5) {
        index += FAMI32_MMC5_CHANNELS;
    }
    return index;
}

int n163_channel_index_for_ext(uint8_t ext_chip) {
    if ((ext_chip & FTM_EXT_N163) == 0) {
        return -1;
    }
    int index = FAMI32_BASE_CHANNELS;
    if (ext_chip & FTM_EXT_VRC6) {
        index += FAMI32_VRC6_CHANNELS;
    }
    if (ext_chip & FTM_EXT_VRC7) {
        index += FAMI32_VRC7_CHANNELS;
    }
    if (ext_chip & FTM_EXT_MMC5) {
        index += FAMI32_MMC5_CHANNELS;
    }
    if (ext_chip & FTM_EXT_FDS) {
        index += FAMI32_FDS_CHANNELS;
    }
    return index;
}

void fill_channel_layout(HEADER_BLOCK &header, uint8_t ext_chip, uint8_t n163_channels) {
    memset(header.ch_id, 0, sizeof(header.ch_id));

    header.ch_id[0] = 0;
    header.ch_id[1] = 1;
    header.ch_id[2] = 2;
    header.ch_id[3] = 3;
    header.ch_id[4] = 4;

    int vrc6_index = vrc6_channel_index_for_ext(ext_chip);
    if (vrc6_index >= 0) {
        for (uint8_t i = 0; i < FAMI32_VRC6_CHANNELS; ++i) {
            header.ch_id[vrc6_index + i] = FTM_CH_ID_VRC6_PULSE1 + i;
        }
    }

    if (ext_chip & FTM_EXT_VRC7) {
        int vrc7_index = vrc7_channel_index_for_ext(ext_chip);
        for (uint8_t i = 0; i < FAMI32_VRC7_CHANNELS; ++i) {
            header.ch_id[vrc7_index + i] = 20 + i;
        }
    }

    int mmc5_index = mmc5_channel_index_for_ext(ext_chip);
    if (mmc5_index >= 0) {
        for (uint8_t i = 0; i < FAMI32_MMC5_CHANNELS; ++i) {
            header.ch_id[mmc5_index + i] = FTM_CH_ID_MMC5_SQUARE1 + i;
        }
    }

    int fds_index = fds_channel_index_for_ext(ext_chip);
    if (fds_index >= 0 && fds_index < FAMI32_MAX_CHANNELS) {
        header.ch_id[fds_index] = FTM_CH_ID_FDS;
    }

    int n163_index = n163_channel_index_for_ext(ext_chip);
    if (n163_index >= 0) {
        uint8_t count = clamp_n163_channels(n163_channels);
        for (uint8_t i = 0; i < count && n163_index + i < FAMI32_MAX_CHANNELS; ++i) {
            header.ch_id[n163_index + i] = FTM_CH_ID_N163_CH1 + i;
        }
    }
}

uint32_t fds_sequence_payload_size(const fds_sequence_t &seq) {
    uint32_t length = seq.length;
    if (length > FAMI32_FDS_SEQUENCE_MAX) {
        length = FAMI32_FDS_SEQUENCE_MAX;
    }
    return 1 + 4 + 4 + 4 + length;
}

uint32_t instrument_payload_size(const instrument_t &inst) {
    if (inst.type == INST_VRC7) {
        return 4 + 8;
    }
    if (inst.type == INST_VRC6) {
        return 4 + (2 * FTM_SEQUENCE_TYPES);
    }
    if (inst.type == INST_FDS) {
        uint32_t size = FAMI32_FDS_WAVE_SIZE + FAMI32_FDS_MOD_SIZE + (4 * 3);
        for (uint8_t i = 0; i < FAMI32_FDS_SEQUENCE_TYPES; ++i) {
            size += fds_sequence_payload_size(inst.fds_seq[i]);
        }
        return size;
    }
    if (inst.type == INST_N163) {
        uint32_t wave_size = inst.n163_wave_size;
        if (wave_size > FAMI32_N163_WAVE_SIZE) {
            wave_size = FAMI32_N163_WAVE_SIZE;
        }
        uint32_t wave_count = inst.n163_wave_count;
        if (wave_count < 1) {
            wave_count = 1;
        }
        if (wave_count > FAMI32_N163_WAVE_MAX) {
            wave_count = FAMI32_N163_WAVE_MAX;
        }
        return 4 + (2 * FTM_SEQUENCE_TYPES) + (4 * 3) + (wave_size * wave_count);
    }
    return 4 + (2 * FTM_SEQUENCE_TYPES) + (3 * FTM_DPCM_NOTES);
}

void write_fds_sequence(FILE *file, const fds_sequence_t &seq) {
    uint8_t length = seq.length;
    if (length > FAMI32_FDS_SEQUENCE_MAX) {
        length = FAMI32_FDS_SEQUENCE_MAX;
    }
    write_u8(file, length);
    write_u32(file, seq.loop);
    write_u32(file, seq.release);
    write_u32(file, seq.setting);
    fwrite(seq.data, 1, length, file);
}

bool read_fds_sequence(FILE *file, fds_sequence_t &seq) {
    uint8_t length = 0;
    if (!read_u8(file, &length)) {
        return false;
    }
    seq.length = length > FAMI32_FDS_SEQUENCE_MAX ? FAMI32_FDS_SEQUENCE_MAX : length;
    uint32_t value = 0;
    read_u32(file, &value);
    seq.loop = value;
    read_u32(file, &value);
    seq.release = value;
    read_u32(file, &value);
    seq.setting = value;
    fread(seq.data, 1, seq.length, file);
    if (length > seq.length) {
        fseek(file, length - seq.length, SEEK_CUR);
    }
    return true;
}

void init_sequence_block(SEQUENCES_BLOCK &block, const char *id) {
    memset(block.id, 0, sizeof(block.id));
    strncpy(block.id, id, sizeof(block.id) - 1);
    block.version = 6;
    block.size = 4;
    block.sequ_num = 0;
}

bool read_sequence_block_header(FILE *file, SEQUENCES_BLOCK &block, const char *id) {
    if (fread(&block, 1, sizeof(block), file) != sizeof(block)) {
        init_sequence_block(block, id);
        return false;
    }
    if (strncmp(block.id, id, sizeof(block.id))) {
        init_sequence_block(block, id);
        fseek(file, -sizeof(block), SEEK_CUR);
        return false;
    }
    return true;
}

void write_sequence_bank(FILE *file, SEQUENCES_BLOCK &block, SequenceBank &bank, const char *id) {
    init_sequence_block(block, id);
    size_t block_start_addr = ftell(file);
    int sequ_num = 0;
    fwrite(&block, sizeof(block), 1, file);

    size_t tol_size = 4;
    for (uint32_t type = 0; type < FTM_SEQUENCE_TYPES; type++) {
        for (int i = 0; i < bank[type].size(); i++) {
            if (bank[type][i].length) {
                bank[type][i].type = type;
                bank[type][i].index = i;
                write_u32(file, bank[type][i].index);
                write_u32(file, bank[type][i].type);
                write_u8(file, bank[type][i].length);
                write_u32(file, bank[type][i].loop);
                fwrite(bank[type][i].data.data(), 1, bank[type][i].length, file);
                tol_size += 13 + bank[type][i].length;
                sequ_num++;
            }
        }
    }
    for (uint32_t type = 0; type < FTM_SEQUENCE_TYPES; type++) {
        for (int i = 0; i < bank[type].size(); i++) {
            if (bank[type][i].length) {
                fwrite(&bank[type][i].release, sizeof(uint32_t), 1, file);
                fwrite(&bank[type][i].setting, sizeof(uint32_t), 1, file);
                tol_size += 4 * 2;
            }
        }
    }

    size_t block_end_addr = ftell(file);
    block.size = tol_size;
    block.sequ_num = sequ_num;
    fseek(file, block_start_addr, SEEK_SET);
    fwrite(&block, sizeof(block), 1, file);
    fseek(file, block_end_addr, SEEK_SET);
}

void read_sequence_bank(FILE *file, const SEQUENCES_BLOCK &block, SequenceBank &bank) {
    bank.clear();
    bank.resize(FTM_SEQUENCE_TYPES);
    if (block.sequ_num == 0) {
        return;
    }
    std::vector<std::vector<uint32_t>> index_map(block.sequ_num, std::vector<uint32_t>(2, 0));
    DBG_PRINTF("\nREADING %.16s, COUNT=%ld\n", block.id, block.sequ_num);
    for (int i = 0; i < block.sequ_num; i++) {
        sequences_t sequ_tmp;
        read_u32(file, &sequ_tmp.index);
        read_u32(file, &sequ_tmp.type);
        read_u8(file, &sequ_tmp.length);
        read_u32(file, &sequ_tmp.loop);
        sequ_tmp.data.resize(sequ_tmp.length);
        fread(sequ_tmp.data.data(), 1, sequ_tmp.length, file);
        if (sequ_tmp.type < FTM_SEQUENCE_TYPES) {
            if (sequ_tmp.index >= bank[sequ_tmp.type].size()) {
                bank[sequ_tmp.type].resize(sequ_tmp.index + 1);
            }
            bank[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
            index_map[i][0] = sequ_tmp.type;
            index_map[i][1] = sequ_tmp.index;
        }
    }
    for (int i = 0; i < block.sequ_num; i++) {
        uint32_t type = index_map[i][0];
        uint32_t index = index_map[i][1];
        fread(&bank[type][index].release, sizeof(uint32_t), 1, file);
        fread(&bank[type][index].setting, sizeof(uint32_t), 1, file);
    }
}

} // namespace

FTM_FILE::FTM_FILE() {
    new_ftm();
}

void FTM_FILE::new_ftm() {
    memset(current_file, 0, sizeof(current_file));

    FTM_HEADER hd_new;
    PARAMS_BLOCK pr_new;
    INFO_BLOCK nf_new;
    HEADER_BLOCK he_new;
    INSTRUMENT_BLOCK inst_new;
    SEQUENCES_BLOCK seq_new;
    SEQUENCES_BLOCK vrc6_seq_new;
    SEQUENCES_BLOCK n163_seq_new;
    FRAME_BLOCK fr_new;
    PATTERN_BLOCK pt_new;
    DPCM_SAMPLE_BLOCK dpcm_new;

    header = hd_new;
    pr_block = pr_new;
    nf_block = nf_new;
    he_block = he_new;
    inst_block = inst_new;
    seq_block = seq_new;
    vrc6_seq_block = vrc6_seq_new;
    init_sequence_block(vrc6_seq_block, "SEQUENCES_VRC6");
    n163_seq_block = n163_seq_new;
    init_sequence_block(n163_seq_block, "SEQUENCES_N163");
    fr_block = fr_new;
    pt_block = pt_new;
    dpcm_block = dpcm_new;
    fill_channel_layout(he_block, pr_block.ext_chip, pr_block.n163_channels);

    frames.clear();
    new_frame();

    patterns.clear();
    patterns.resize(pr_block.channel);

    unpack_pt.clear();
    unpack_pt.resize(pr_block.channel);

    for (int c = 0; c < pr_block.channel; c++) {
        patterns[c].resize(1);
    }

    for (int c = 0; c < pr_block.channel; c++) {
        unpack_pt[c].resize(1);
        unpack_pt[c][0].resize(fr_block.pat_length);
    }

    dpcm_samples.clear();
    instrument.clear();
    sequences.clear();
    sequences.resize(FTM_SEQUENCE_TYPES);
    vrc6_sequences.clear();
    vrc6_sequences.resize(FTM_SEQUENCE_TYPES);
    n163_sequences.clear();
    n163_sequences.resize(FTM_SEQUENCE_TYPES);
    create_new_inst();
}

void FTM_FILE::create_new_inst() {
    if (instrument.empty()) {
        instrument.emplace_back();
        instrument[0].index = 0;
        for (uint32_t i = 0; i < FTM_SEQUENCE_TYPES; ++i) {
            instrument[0].seq_index[i].seq_index = sequences[i].size();
        }
        inst_block.inst_num = instrument.size();
        return;
    }

    int insert_index = -1;
    for (int i = 0; i < instrument.size() - 1; ++i) {
        if (instrument[i].index + 1 != instrument[i + 1].index) {
            insert_index = instrument[i].index + 1;
            break;
        }
    }

    int new_index = -1;
    if (insert_index != -1) {
        instrument.emplace(instrument.begin() + insert_index);
        instrument[insert_index].index = insert_index;
        new_index = insert_index;
    } else {
        instrument.emplace_back();
        new_index = instrument.size() - 1;
        instrument.back().index = new_index;
    }

    for (uint32_t i = 0; i < FTM_SEQUENCE_TYPES; ++i) {
        instrument[new_index].seq_index[i].seq_index = sequences[i].size();
    }

    inst_block.inst_num = instrument.size();
}

void FTM_FILE::remove_inst(int n) {
    instrument.erase(instrument.begin() + n);
    if (instrument.size() < 1) {
        create_new_inst();
    }
    inst_block.inst_num = instrument.size();
}

void FTM_FILE::set_inst_type(int n, uint8_t type) {
    instrument_t *inst = get_inst(n);
    if (inst == NULL) {
        return;
    }

    if (type == INST_VRC7) {
        inst->type = INST_VRC7;
        inst->seq_count = 0;
        if (inst->vrc7_patch > 15) {
            inst->vrc7_patch = 0;
        }
    } else if (type == INST_VRC6) {
        inst->type = INST_VRC6;
        inst->seq_count = FTM_SEQUENCE_TYPES;
        for (uint32_t i = 0; i < FTM_SEQUENCE_TYPES; ++i) {
            if (inst->seq_index[i].seq_index >= vrc6_sequences[i].size()) {
                inst->seq_index[i].seq_index = vrc6_sequences[i].size();
            }
        }
    } else if (type == INST_FDS) {
        inst->type = INST_FDS;
        inst->seq_count = FAMI32_FDS_SEQUENCE_TYPES;
    } else if (type == INST_N163) {
        inst->type = INST_N163;
        inst->seq_count = FTM_SEQUENCE_TYPES;
        if (inst->n163_wave_size < 1 || inst->n163_wave_size > FAMI32_N163_WAVE_SIZE) {
            inst->n163_wave_size = FAMI32_N163_WAVE_SIZE;
        }
        if (inst->n163_wave_count < 1 || inst->n163_wave_count > FAMI32_N163_WAVE_MAX) {
            inst->n163_wave_count = 1;
        }
        for (uint32_t i = 0; i < FTM_SEQUENCE_TYPES; ++i) {
            if (inst->seq_index[i].seq_index >= n163_sequences[i].size()) {
                inst->seq_index[i].seq_index = n163_sequences[i].size();
            }
        }
    } else {
        inst->type = INST_2A03;
        inst->seq_count = FTM_SEQUENCE_TYPES;
        for (uint32_t i = 0; i < FTM_SEQUENCE_TYPES; ++i) {
            if (inst->seq_index[i].seq_index >= sequences[i].size()) {
                inst->seq_index[i].seq_index = sequences[i].size();
            }
        }
    }
}

uint32_t FTM_FILE::get_channel_count() const {
    return pr_block.channel;
}

bool FTM_FILE::vrc7_enabled() const {
    return (pr_block.ext_chip & FTM_EXT_VRC7) != 0;
}

bool FTM_FILE::is_vrc7_channel(int c) const {
    int first = vrc7_channel_index_for_ext(pr_block.ext_chip);
    return first >= 0 && c >= first && c < first + FAMI32_VRC7_CHANNELS;
}

int FTM_FILE::vrc7_channel_index() const {
    return vrc7_channel_index_for_ext(pr_block.ext_chip);
}

void FTM_FILE::set_vrc7_enabled(bool enabled) {
    if (enabled) {
        pr_block.ext_chip |= FTM_EXT_VRC7;
    } else {
        pr_block.ext_chip &= ~FTM_EXT_VRC7;
    }
    apply_channel_layout();
}

bool FTM_FILE::vrc6_enabled() const {
    return (pr_block.ext_chip & FTM_EXT_VRC6) != 0;
}

void FTM_FILE::set_vrc6_enabled(bool enabled) {
    if (enabled) {
        pr_block.ext_chip |= FTM_EXT_VRC6;
    } else {
        pr_block.ext_chip &= ~FTM_EXT_VRC6;
    }
    apply_channel_layout();
}

int FTM_FILE::vrc6_channel_index() const {
    return vrc6_channel_index_for_ext(pr_block.ext_chip);
}

bool FTM_FILE::is_vrc6_channel(int c) const {
    int first = vrc6_channel_index();
    return first >= 0 && c >= first && c < first + FAMI32_VRC6_CHANNELS;
}

bool FTM_FILE::fds_enabled() const {
    return (pr_block.ext_chip & FTM_EXT_FDS) != 0;
}

void FTM_FILE::set_fds_enabled(bool enabled) {
    if (enabled) {
        pr_block.ext_chip |= FTM_EXT_FDS;
    } else {
        pr_block.ext_chip &= ~FTM_EXT_FDS;
    }
    apply_channel_layout();
}

int FTM_FILE::fds_channel_index() const {
    return fds_channel_index_for_ext(pr_block.ext_chip);
}

bool FTM_FILE::is_fds_channel(int c) const {
    return fds_enabled() && c == fds_channel_index();
}

bool FTM_FILE::mmc5_enabled() const {
    return (pr_block.ext_chip & FTM_EXT_MMC5) != 0;
}

void FTM_FILE::set_mmc5_enabled(bool enabled) {
    if (enabled) {
        pr_block.ext_chip |= FTM_EXT_MMC5;
    } else {
        pr_block.ext_chip &= ~FTM_EXT_MMC5;
    }
    apply_channel_layout();
}

int FTM_FILE::mmc5_channel_index() const {
    return mmc5_channel_index_for_ext(pr_block.ext_chip);
}

bool FTM_FILE::is_mmc5_channel(int c) const {
    int first = mmc5_channel_index();
    return first >= 0 && c >= first && c < first + FAMI32_MMC5_CHANNELS;
}

bool FTM_FILE::n163_enabled() const {
    return (pr_block.ext_chip & FTM_EXT_N163) != 0;
}

void FTM_FILE::set_n163_enabled(bool enabled) {
    if (enabled) {
        pr_block.ext_chip |= FTM_EXT_N163;
        pr_block.n163_channels = clamp_n163_channels(pr_block.n163_channels);
    } else {
        pr_block.ext_chip &= ~FTM_EXT_N163;
    }
    apply_channel_layout();
}

int FTM_FILE::n163_channel_index() const {
    return n163_channel_index_for_ext(pr_block.ext_chip);
}

uint8_t FTM_FILE::n163_channel_count() const {
    return clamp_n163_channels(pr_block.n163_channels);
}

void FTM_FILE::set_n163_channel_count(uint8_t count) {
    pr_block.n163_channels = clamp_n163_channels(count);
    if (n163_enabled()) {
        apply_channel_layout();
    }
}

bool FTM_FILE::is_n163_channel(int c) const {
    int first = n163_channel_index();
    uint8_t count = n163_channel_count();
    return first >= 0 && c >= first && c < first + count;
}

void FTM_FILE::apply_channel_layout() {
    uint32_t new_count = channel_count_for_ext(pr_block.ext_chip, pr_block.n163_channels);
    if (new_count > FAMI32_MAX_CHANNELS) {
        new_count = FAMI32_MAX_CHANNELS;
    }
    resize_channel_storage(new_count);
    fill_channel_layout(he_block, pr_block.ext_chip, pr_block.n163_channels);
}

void FTM_FILE::resize_channel_storage(uint32_t new_count) {
    if (new_count < FAMI32_BASE_CHANNELS) {
        new_count = FAMI32_BASE_CHANNELS;
    }
    if (new_count > FAMI32_MAX_CHANNELS) {
        new_count = FAMI32_MAX_CHANNELS;
    }

    uint32_t old_count = pr_block.channel;
    pr_block.channel = new_count;

    for (size_t f = 0; f < frames.size(); ++f) {
        uint8_t default_pattern = static_cast<uint8_t>(f & 0xFF);
        frames[f].resize(new_count, default_pattern);
    }

    patterns.resize(new_count);
    unpack_pt.resize(new_count);

    for (uint32_t c = 0; c < new_count; ++c) {
        if (patterns[c].empty()) {
            patterns[c].resize(1);
        }
        if (unpack_pt[c].empty()) {
            unpack_pt[c].resize(1);
        }

        uint32_t max_pattern = 0;
        for (size_t f = 0; f < frames.size(); ++f) {
            if (c < frames[f].size() && frames[f][c] > max_pattern) {
                max_pattern = frames[f][c];
            }
        }
        if (unpack_pt[c].size() <= max_pattern) {
            unpack_pt[c].resize(max_pattern + 1);
        }
        for (size_t p = 0; p < unpack_pt[c].size(); ++p) {
            unpack_pt[c][p].resize(fr_block.pat_length);
        }
    }

    if (new_count < old_count) {
        patterns.resize(new_count);
        unpack_pt.resize(new_count);
    }
}

int FTM_FILE::open_ftm(const char *filename) {
    ftm_file = fopen(filename, "rb");
    if (ftm_file == NULL) {
        perror("Error opening file");
        return FILE_OPEN_ERROR;
    }
    fread(&header, 1, sizeof(header), ftm_file);
    if (strncmp(header.id, "FamiTracker Module", 18)) {
        DBG_PRINTF("HEADER: %.16s\n", header.id);
        DBG_PRINTF("This is not a valid FTM file!\n");
        header = FTM_HEADER();
        fclose(ftm_file);
        return FTM_NOT_VALID;
    };
    if (header.version != 0x0440) {
        DBG_PRINTF("Only FTM file version 0x0440 is supported\nVERSION: 0x%lX\n", (unsigned long)header.version);
        header = FTM_HEADER();
        fclose(ftm_file);
        return NO_SUPPORT_VERSION;
    }
    DBG_PRINTF("\nOpen %.18s\n", header.id);
    DBG_PRINTF("VERSION: 0x%lX\n", (unsigned long)header.version);
    strncpy(current_file, filename, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';
    return 0;
}

int FTM_FILE::save_as_ftm(const char *filename) {
    ftm_file = fopen(filename, "wb");

    if (ftm_file == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(ftm_file, 0, SEEK_SET);

    write_file_header();
    write_param_block();

    write_info_block();

    write_header_block();
    write_instrument_block();
    write_instrument_data();

    write_sequences();

    write_frame();

    write_pattern();

    write_dpcm();
    if (vrc6_enabled()) {
        write_vrc6_sequences();
    }
    if (n163_enabled()) {
        write_n163_sequences();
    }

    fwrite("\0END", 1, 4, ftm_file);

    fclose(ftm_file);

    strncpy(current_file, filename, sizeof(current_file) - 1);
    current_file[sizeof(current_file) - 1] = '\0';

    return 0;
}

int FTM_FILE::save_ftm() {
    save_as_ftm(current_file);
    return 0;
}

void FTM_FILE::write_file_header() {
    fwrite(&header, sizeof(header), 1, ftm_file);
}

void FTM_FILE::write_param_block() {
    pr_block.version = 6;
    pr_block.n163_channels = clamp_n163_channels(pr_block.n163_channels);
    pr_block.size = 29 + (n163_enabled() ? 4 : 0);
    fwrite(pr_block.id, 1, 16, ftm_file);
    write_u32(ftm_file, pr_block.version);
    write_u32(ftm_file, pr_block.size);
    write_u8(ftm_file, pr_block.ext_chip);
    write_u32(ftm_file, pr_block.channel);
    write_u32(ftm_file, pr_block.machine);
    write_u32(ftm_file, pr_block.e_speed);
    write_u32(ftm_file, pr_block.v_style);
    write_u32(ftm_file, pr_block.hl1);
    write_u32(ftm_file, pr_block.hl2);
    if (n163_enabled()) {
        write_u32(ftm_file, pr_block.n163_channels);
    }
    write_u32(ftm_file, pr_block.s_split);
}

int FTM_FILE::read_param_block() {
    pr_block = PARAMS_BLOCK();
    fread(pr_block.id, 1, 16, ftm_file);
    uint32_t value = 0;
    read_u32(ftm_file, &value);
    pr_block.version = value;
    read_u32(ftm_file, &value);
    pr_block.size = value;

    size_t payload_start = ftell(ftm_file);
    read_u8(ftm_file, &pr_block.ext_chip);
    read_u32(ftm_file, &value);
    pr_block.channel = value;
    read_u32(ftm_file, &value);
    pr_block.machine = value;
    read_u32(ftm_file, &value);
    pr_block.e_speed = value;
    read_u32(ftm_file, &value);
    pr_block.v_style = value;
    read_u32(ftm_file, &value);
    pr_block.hl1 = value;
    read_u32(ftm_file, &value);
    pr_block.hl2 = value;
    if (pr_block.version >= 5 && (pr_block.ext_chip & FTM_EXT_N163)) {
        read_u32(ftm_file, &value);
        pr_block.n163_channels = value;
    }
    pr_block.n163_channels = clamp_n163_channels(pr_block.n163_channels);
    if (pr_block.version >= 6) {
        read_u32(ftm_file, &value);
        pr_block.s_split = value;
    }
    size_t payload_read = ftell(ftm_file) - payload_start;
    if (pr_block.size > payload_read) {
        fseek(ftm_file, pr_block.size - payload_read, SEEK_CUR);
    }

    if (pr_block.ext_chip & ~(FTM_EXT_VRC6 | FTM_EXT_VRC7 | FTM_EXT_FDS | FTM_EXT_MMC5 | FTM_EXT_N163)) {
        DBG_PRINTF("Only 2A03, VRC6, VRC7, FDS, MMC5 and N163 FTM files are supported\n");
        return NO_SUPPORT_EXTCHIP;
    }

    uint32_t expected_channels = channel_count_for_ext(pr_block.ext_chip, pr_block.n163_channels);
    if (expected_channels > FAMI32_MAX_CHANNELS) {
        return NO_SUPPORT_EXTCHIP;
    }
    if (pr_block.channel < expected_channels) {
        pr_block.channel = expected_channels;
    }
    if (pr_block.channel > FAMI32_MAX_CHANNELS) {
        DBG_PRINTF("Too many channels in FTM file: %ld\n", pr_block.channel);
        return NO_SUPPORT_EXTCHIP;
    }

    DBG_PRINTF("\nPARAMS HEADER: %s\n", pr_block.id);
    DBG_PRINTF("VERSION: %ld\n", pr_block.version);
    DBG_PRINTF("SIZE: %ld\n", pr_block.size);
    DBG_PRINTF("EXTRA_CHIP: 0x%X\n", pr_block.ext_chip);
    DBG_PRINTF("CHANNELS: %ld\n", pr_block.channel);
    DBG_PRINTF("N163_CHANNELS: %ld\n", pr_block.n163_channels);
    DBG_PRINTF("MACHINE: 0x%lX\n", pr_block.machine);
    DBG_PRINTF("E_SPEED: %ld\n", pr_block.e_speed);
    DBG_PRINTF("V_STYLE: %ld\n", pr_block.v_style);
    DBG_PRINTF("HIGHLINE1: %ld\n", pr_block.hl1);
    DBG_PRINTF("HIGHLINE2: %ld\n", pr_block.hl2);
    return 0;
}

void FTM_FILE::read_info_block() {
    fread(&nf_block, 1, sizeof(nf_block), ftm_file);

    DBG_PRINTF("\nINFO HEADER: %s\n", nf_block.id);
    DBG_PRINTF("VERSION: %ld\n", nf_block.version);
    DBG_PRINTF("SIZE: %ld\n", nf_block.size);
    DBG_PRINTF("TITLE: %s\n", nf_block.title);
    DBG_PRINTF("AUTHOR: %s\n", nf_block.author);
    DBG_PRINTF("COPYRIGHT: %s\n", nf_block.copyright);
}

void FTM_FILE::write_info_block() {
    nf_block.size = 96;
    fwrite(&nf_block, sizeof(nf_block), 1, ftm_file);
}

int FTM_FILE::read_header_block() {
    fread(&he_block, 1, 25, ftm_file);

    uint32_t name_bytes = he_block.size - (pr_block.channel * 2) - 1;
    uint32_t copy_bytes = name_bytes < sizeof(he_block.name) - 1 ? name_bytes : sizeof(he_block.name) - 1;
    memset(he_block.name, 0, sizeof(he_block.name));
    fread(he_block.name, 1, copy_bytes, ftm_file);
    if (name_bytes > copy_bytes) {
        fseek(ftm_file, name_bytes - copy_bytes, SEEK_CUR);
    }
    if (he_block.track_num > 0) {
        DBG_PRINTF("Multi-track FTM files are not supported\n");
        return NO_SUPPORT_MULTITRACK;
    }

    for (uint32_t i = 0; i < pr_block.channel; i++) {
        fread(&he_block.ch_id[i], 1, 1, ftm_file);
        fread(&he_block.ch_fx[i], 1, 1, ftm_file);
    }
    apply_channel_layout();

    DBG_PRINTF("\nMETADATA HEADER: %s\n", he_block.id);
    DBG_PRINTF("VERSION: %ld\n", he_block.version);
    DBG_PRINTF("SIZE: %ld\n", he_block.size);
    DBG_PRINTF("TRACK: %d\n", he_block.track_num + 1);
    DBG_PRINTF("NAME: %s\n", he_block.name);
    for (uint8_t i = 0; i < pr_block.channel; i++) {
        DBG_PRINTF("CH%d: EX_FX*%d  ", he_block.ch_id[i], he_block.ch_fx[i]);
    }

    DBG_PRINTF("\n");
    return 0;
}

void FTM_FILE::write_header_block() {
    size_t name_len = bounded_strlen(he_block.name, sizeof(he_block.name) - 1);
    he_block.size = static_cast<uint32_t>(1 + name_len + 1 + (pr_block.channel * 2));
    fwrite(&he_block, 1, 25, ftm_file);
    fwrite(he_block.name, 1, name_len + 1, ftm_file);

    for (int i = 0; i < pr_block.channel; i++) {
        fwrite(&he_block.ch_id[i], 1, 1, ftm_file);
        fwrite(&he_block.ch_fx[i], 1, 1, ftm_file);
    }
}

void FTM_FILE::read_instrument_block() {
    fread(&inst_block, 1, sizeof(inst_block), ftm_file);
    DBG_PRINTF("\nINSTRUMENT HEADER: %s\n", inst_block.id);
    DBG_PRINTF("VERSION: %ld\n", inst_block.version);
    DBG_PRINTF("SIZE: %ld\n", inst_block.size);
    DBG_PRINTF("NUM_INST: 0x%lX\n", inst_block.inst_num);
}

void FTM_FILE::write_instrument_block() {
    uint32_t valid_count = 0;
    size_t tol_size = 4;
    for (size_t i = 0; i < instrument.size(); i++) {
        if (!is_valid_instrument_slot(instrument, i)) {
            continue;
        }
        valid_count++;
        tol_size += 4 + 1 + instrument_payload_size(instrument[i]) + 4 + instrument_name_length(instrument[i]);
    }
    inst_block.inst_num = valid_count;
    inst_block.size = static_cast<uint32_t>(tol_size);
    fwrite(&inst_block, sizeof(inst_block), 1, ftm_file);
}

void FTM_FILE::read_instrument_data() {
    instrument.clear();
    DBG_PRINTF("\nREADING INSTRUMENT, COUNT=%ld\n", inst_block.inst_num);
    for (uint32_t i = 0; i < inst_block.inst_num; i++) {
        instrument_t inst_tmp;
        DBG_PRINTF("\nREADING INSTRUMENT #%02lX...\n", (unsigned long)i);

        uint32_t u32_value = 0;
        uint8_t u8_value = 0;

        if (!read_u32(ftm_file, &u32_value)) {
            break;
        }
        inst_tmp.index = u32_value;
        read_u8(ftm_file, &u8_value);
        inst_tmp.type = u8_value;

        if (inst_tmp.type == INST_VRC7) {
            read_u32(ftm_file, &u32_value);
            inst_tmp.vrc7_patch = u32_value & 0x0F;
            for (uint8_t r = 0; r < 8; ++r) {
                read_u8(ftm_file, &inst_tmp.vrc7_regs[r]);
            }
            inst_tmp.seq_count = 0;
        } else if (inst_tmp.type == INST_FDS) {
            fread(inst_tmp.fds_wave, 1, FAMI32_FDS_WAVE_SIZE, ftm_file);
            fread(inst_tmp.fds_mod, 1, FAMI32_FDS_MOD_SIZE, ftm_file);
            read_u32(ftm_file, &u32_value);
            inst_tmp.fds_mod_speed = u32_value;
            read_u32(ftm_file, &u32_value);
            inst_tmp.fds_mod_depth = u32_value;
            read_u32(ftm_file, &u32_value);
            inst_tmp.fds_mod_delay = u32_value;
            for (uint8_t seq = 0; seq < FAMI32_FDS_SEQUENCE_TYPES; ++seq) {
                read_fds_sequence(ftm_file, inst_tmp.fds_seq[seq]);
            }
            inst_tmp.seq_count = FAMI32_FDS_SEQUENCE_TYPES;
        } else if (inst_tmp.type == INST_N163) {
            read_u32(ftm_file, &u32_value);
            inst_tmp.seq_count = u32_value;
            if (inst_tmp.seq_count > FTM_SEQUENCE_TYPES) {
                inst_tmp.seq_count = FTM_SEQUENCE_TYPES;
            }
            for (uint32_t seq = 0; seq < FTM_SEQUENCE_TYPES; ++seq) {
                uint8_t enable = 0;
                uint8_t index = 0;
                read_u8(ftm_file, &enable);
                read_u8(ftm_file, &index);
                inst_tmp.seq_index[seq].enable = enable != 0;
                inst_tmp.seq_index[seq].seq_index = index;
            }
            read_u32(ftm_file, &u32_value);
            inst_tmp.n163_wave_size = u32_value;
            read_u32(ftm_file, &u32_value);
            inst_tmp.n163_wave_pos = u32_value;
            read_u32(ftm_file, &u32_value);
            inst_tmp.n163_wave_count = u32_value;
            uint32_t read_wave_size = inst_tmp.n163_wave_size;
            uint32_t read_wave_count = inst_tmp.n163_wave_count;
            if (inst_tmp.n163_wave_size < 1) inst_tmp.n163_wave_size = 1;
            if (inst_tmp.n163_wave_size > FAMI32_N163_WAVE_SIZE) inst_tmp.n163_wave_size = FAMI32_N163_WAVE_SIZE;
            if (inst_tmp.n163_wave_count < 1) inst_tmp.n163_wave_count = 1;
            if (inst_tmp.n163_wave_count > FAMI32_N163_WAVE_MAX) inst_tmp.n163_wave_count = FAMI32_N163_WAVE_MAX;
            for (uint32_t wave = 0; wave < read_wave_count; ++wave) {
                for (uint32_t sample = 0; sample < read_wave_size; ++sample) {
                    uint8_t value = 0;
                    read_u8(ftm_file, &value);
                    if (wave < FAMI32_N163_WAVE_MAX && sample < FAMI32_N163_WAVE_SIZE) {
                        inst_tmp.n163_wave[wave][sample] = value & 0x0F;
                    }
                }
            }
        } else if (inst_tmp.type == INST_VRC6) {
            read_u32(ftm_file, &u32_value);
            inst_tmp.seq_count = u32_value;
            if (inst_tmp.seq_count > FTM_SEQUENCE_TYPES) {
                inst_tmp.seq_count = FTM_SEQUENCE_TYPES;
            }
            for (uint32_t seq = 0; seq < FTM_SEQUENCE_TYPES; ++seq) {
                uint8_t enable = 0;
                uint8_t index = 0;
                read_u8(ftm_file, &enable);
                read_u8(ftm_file, &index);
                inst_tmp.seq_index[seq].enable = enable != 0;
                inst_tmp.seq_index[seq].seq_index = index;
            }
        } else {
            inst_tmp.type = INST_2A03;
            read_u32(ftm_file, &u32_value);
            inst_tmp.seq_count = u32_value;
            if (inst_tmp.seq_count > FTM_SEQUENCE_TYPES) {
                inst_tmp.seq_count = FTM_SEQUENCE_TYPES;
            }
            for (uint32_t seq = 0; seq < FTM_SEQUENCE_TYPES; ++seq) {
                uint8_t enable = 0;
                uint8_t index = 0;
                read_u8(ftm_file, &enable);
                read_u8(ftm_file, &index);
                inst_tmp.seq_index[seq].enable = enable != 0;
                inst_tmp.seq_index[seq].seq_index = index;
            }
            for (uint32_t note = 0; note < FTM_DPCM_NOTES; ++note) {
                uint8_t pitch = 0;
                read_u8(ftm_file, &inst_tmp.dpcm[note].index);
                read_u8(ftm_file, &pitch);
                read_u8(ftm_file, &inst_tmp.dpcm[note].d_counte);
                inst_tmp.dpcm[note].pitch = pitch & 0x0F;
                inst_tmp.dpcm[note].loop = (pitch & 0x80) ? 8 : 0;
            }
        }

        read_u32(ftm_file, &u32_value);
        inst_tmp.name_len = u32_value;
        uint32_t copy_len = inst_tmp.name_len < sizeof(inst_tmp.name) - 1 ? inst_tmp.name_len : sizeof(inst_tmp.name) - 1;
        memset(inst_tmp.name, 0, sizeof(inst_tmp.name));
        fread(inst_tmp.name, 1, copy_len, ftm_file);
        if (inst_tmp.name_len > copy_len) {
            fseek(ftm_file, inst_tmp.name_len - copy_len, SEEK_CUR);
        }
        inst_tmp.name_len = copy_len;

        DBG_PRINTF("%02lX->NAME: %s\n", inst_tmp.index, inst_tmp.name);
        DBG_PRINTF("TYPE: 0x%X\n", inst_tmp.type);
        if (inst_tmp.type == INST_VRC7) {
            DBG_PRINTF("VRC7 PATCH: %ld\n", inst_tmp.vrc7_patch);
        } else if (inst_tmp.type == INST_FDS) {
            DBG_PRINTF("FDS MOD SPD:%ld DEP:%ld DLY:%ld\n", inst_tmp.fds_mod_speed, inst_tmp.fds_mod_depth, inst_tmp.fds_mod_delay);
        } else if (inst_tmp.type == INST_N163) {
            DBG_PRINTF("N163 WAVE SIZE:%ld POS:%ld COUNT:%ld\n", inst_tmp.n163_wave_size, inst_tmp.n163_wave_pos, inst_tmp.n163_wave_count);
        } else {
            DBG_PRINTF("SEQU COUNT: %ld\n", inst_tmp.seq_count);
        }

        if (inst_tmp.index >= instrument.size()) {
            instrument.resize(inst_tmp.index + 1);
        }
        instrument[inst_tmp.index] = inst_tmp;
    }
    inst_block.inst_num = instrument.size();
}

void FTM_FILE::write_instrument_data() {
    for (size_t i = 0; i < instrument.size(); i++) {
        if (!is_valid_instrument_slot(instrument, i)) {
            continue;
        }
        instrument_t &inst = instrument[i];
        if (inst.type != INST_VRC7 && inst.type != INST_VRC6 && inst.type != INST_FDS && inst.type != INST_N163) {
            inst.type = INST_2A03;
            inst.seq_count = FTM_SEQUENCE_TYPES;
        }
        instrument_name_length(inst);

        write_u32(ftm_file, inst.index);
        write_u8(ftm_file, inst.type);
        if (inst.type == INST_VRC7) {
            write_u32(ftm_file, inst.vrc7_patch & 0x0F);
            for (uint8_t r = 0; r < 8; ++r) {
                write_u8(ftm_file, inst.vrc7_regs[r]);
            }
        } else if (inst.type == INST_VRC6) {
            write_u32(ftm_file, inst.seq_count);
            for (uint32_t seq = 0; seq < FTM_SEQUENCE_TYPES; ++seq) {
                write_u8(ftm_file, inst.seq_index[seq].enable ? 1 : 0);
                write_u8(ftm_file, inst.seq_index[seq].seq_index);
            }
        } else if (inst.type == INST_FDS) {
            fwrite(inst.fds_wave, 1, FAMI32_FDS_WAVE_SIZE, ftm_file);
            fwrite(inst.fds_mod, 1, FAMI32_FDS_MOD_SIZE, ftm_file);
            write_u32(ftm_file, inst.fds_mod_speed);
            write_u32(ftm_file, inst.fds_mod_depth);
            write_u32(ftm_file, inst.fds_mod_delay);
            for (uint8_t seq = 0; seq < FAMI32_FDS_SEQUENCE_TYPES; ++seq) {
                write_fds_sequence(ftm_file, inst.fds_seq[seq]);
            }
        } else if (inst.type == INST_N163) {
            inst.seq_count = FTM_SEQUENCE_TYPES;
            if (inst.n163_wave_size < 1) inst.n163_wave_size = 1;
            if (inst.n163_wave_size > FAMI32_N163_WAVE_SIZE) inst.n163_wave_size = FAMI32_N163_WAVE_SIZE;
            if (inst.n163_wave_count < 1) inst.n163_wave_count = 1;
            if (inst.n163_wave_count > FAMI32_N163_WAVE_MAX) inst.n163_wave_count = FAMI32_N163_WAVE_MAX;
            if (inst.n163_wave_pos > 127) inst.n163_wave_pos = 127;
            write_u32(ftm_file, inst.seq_count);
            for (uint32_t seq = 0; seq < FTM_SEQUENCE_TYPES; ++seq) {
                write_u8(ftm_file, inst.seq_index[seq].enable ? 1 : 0);
                write_u8(ftm_file, inst.seq_index[seq].seq_index);
            }
            write_u32(ftm_file, inst.n163_wave_size);
            write_u32(ftm_file, inst.n163_wave_pos);
            write_u32(ftm_file, inst.n163_wave_count);
            for (uint32_t wave = 0; wave < inst.n163_wave_count; ++wave) {
                for (uint32_t sample = 0; sample < inst.n163_wave_size; ++sample) {
                    write_u8(ftm_file, inst.n163_wave[wave][sample] & 0x0F);
                }
            }
        } else {
            write_u32(ftm_file, inst.seq_count);
            for (uint32_t seq = 0; seq < FTM_SEQUENCE_TYPES; ++seq) {
                write_u8(ftm_file, inst.seq_index[seq].enable ? 1 : 0);
                write_u8(ftm_file, inst.seq_index[seq].seq_index);
            }
            for (uint32_t note = 0; note < FTM_DPCM_NOTES; ++note) {
                write_u8(ftm_file, inst.dpcm[note].index);
                write_u8(ftm_file, make_dpcm_pitch_byte(inst.dpcm[note]));
                write_u8(ftm_file, inst.dpcm[note].d_counte);
            }
        }
        write_u32(ftm_file, inst.name_len);
        fwrite(inst.name, 1, inst.name_len, ftm_file);
    }
}

void FTM_FILE::read_sequences_block() {
    if (!read_sequence_block_header(ftm_file, seq_block, "SEQUENCES")) {
        DBG_PRINTF("NO SEQUENCES!!\n");
        return;
    }
    DBG_PRINTF("\nSEQUENCES HEADER: %s\n", seq_block.id);
    DBG_PRINTF("VERSION: %ld\n", seq_block.version);
    DBG_PRINTF("SIZE: %ld\n", seq_block.size);
    DBG_PRINTF("NUM_SEQU: %ld\n", seq_block.sequ_num);
}

void FTM_FILE::read_vrc6_sequences_block() {
    if (!read_sequence_block_header(ftm_file, vrc6_seq_block, "SEQUENCES_VRC6")) {
        DBG_PRINTF("NO VRC6 SEQUENCES!!\n");
        return;
    }
    DBG_PRINTF("\nVRC6 SEQUENCES HEADER: %s\n", vrc6_seq_block.id);
    DBG_PRINTF("VERSION: %ld\n", vrc6_seq_block.version);
    DBG_PRINTF("SIZE: %ld\n", vrc6_seq_block.size);
    DBG_PRINTF("NUM_SEQU: %ld\n", vrc6_seq_block.sequ_num);
}

void FTM_FILE::write_vrc6_sequences() {
    write_sequence_bank(ftm_file, vrc6_seq_block, vrc6_sequences, "SEQUENCES_VRC6");
}

void FTM_FILE::read_vrc6_sequences_data() {
    read_sequence_bank(ftm_file, vrc6_seq_block, vrc6_sequences);
}

void FTM_FILE::write_n163_sequences() {
    init_sequence_block(n163_seq_block, "SEQUENCES_N163");
    n163_seq_block.version = 1;
    size_t block_start_addr = ftell(ftm_file);
    int sequ_num = 0;
    fwrite(&n163_seq_block, sizeof(n163_seq_block), 1, ftm_file);

    size_t tol_size = 4;
    for (uint32_t type = 0; type < FTM_SEQUENCE_TYPES; type++) {
        for (int i = 0; i < n163_sequences[type].size(); i++) {
            if (n163_sequences[type][i].length) {
                n163_sequences[type][i].type = type;
                n163_sequences[type][i].index = i;
                write_u32(ftm_file, n163_sequences[type][i].index);
                write_u32(ftm_file, n163_sequences[type][i].type);
                write_u8(ftm_file, n163_sequences[type][i].length);
                write_u32(ftm_file, n163_sequences[type][i].loop);
                write_u32(ftm_file, n163_sequences[type][i].release);
                write_u32(ftm_file, n163_sequences[type][i].setting);
                fwrite(n163_sequences[type][i].data.data(), 1, n163_sequences[type][i].length, ftm_file);
                tol_size += 21 + n163_sequences[type][i].length;
                sequ_num++;
            }
        }
    }

    size_t block_end_addr = ftell(ftm_file);
    n163_seq_block.size = tol_size;
    n163_seq_block.sequ_num = sequ_num;
    fseek(ftm_file, block_start_addr, SEEK_SET);
    fwrite(&n163_seq_block, sizeof(n163_seq_block), 1, ftm_file);
    fseek(ftm_file, block_end_addr, SEEK_SET);
}

void FTM_FILE::read_n163_sequences_data() {
    n163_sequences.clear();
    n163_sequences.resize(FTM_SEQUENCE_TYPES);
    if (n163_seq_block.sequ_num == 0) {
        return;
    }

    DBG_PRINTF("\nREADING SEQUENCES_N163, COUNT=%ld\n", n163_seq_block.sequ_num);
    for (int i = 0; i < n163_seq_block.sequ_num; i++) {
        sequences_t sequ_tmp;
        read_u32(ftm_file, &sequ_tmp.index);
        read_u32(ftm_file, &sequ_tmp.type);
        read_u8(ftm_file, &sequ_tmp.length);
        read_u32(ftm_file, &sequ_tmp.loop);
        read_u32(ftm_file, &sequ_tmp.release);
        read_u32(ftm_file, &sequ_tmp.setting);
        sequ_tmp.data.resize(sequ_tmp.length);
        fread(sequ_tmp.data.data(), 1, sequ_tmp.length, ftm_file);
        if (sequ_tmp.type < FTM_SEQUENCE_TYPES) {
            if (sequ_tmp.index >= n163_sequences[sequ_tmp.type].size()) {
                n163_sequences[sequ_tmp.type].resize(sequ_tmp.index + 1);
            }
            n163_sequences[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
        }
    }
}

void FTM_FILE::write_sequences() {
    size_t block_start_addr = ftell(ftm_file);
    int sequ_num = 0;
    fwrite(&seq_block, sizeof(seq_block), 1, ftm_file);

    size_t tol_size = 4;
    for (uint32_t type = 0; type < FTM_SEQUENCE_TYPES; type++) {
        for (int i = 0; i < sequences[type].size(); i++) {
            if (sequences[type][i].length) {
                sequences[type][i].type = type;
                sequences[type][i].index = i;
                write_u32(ftm_file, sequences[type][i].index);
                write_u32(ftm_file, sequences[type][i].type);
                write_u8(ftm_file, sequences[type][i].length);
                write_u32(ftm_file, sequences[type][i].loop);
                fwrite(sequences[type][i].data.data(), 1, sequences[type][i].length, ftm_file);
                tol_size += 13 + sequences[type][i].length;
                sequ_num++;
            }
        }
    }
    for (uint32_t type = 0; type < FTM_SEQUENCE_TYPES; type++) {
        for (int i = 0; i < sequences[type].size(); i++) {
            if (sequences[type][i].length) {
                fwrite(&sequences[type][i].release, sizeof(uint32_t), 1, ftm_file);
                fwrite(&sequences[type][i].setting, sizeof(uint32_t), 1, ftm_file);
                tol_size += 4 * 2;
            }
        }
    }
    size_t block_end_addr = ftell(ftm_file);
    seq_block.size = tol_size;
    seq_block.sequ_num = sequ_num;
    fseek(ftm_file, block_start_addr, SEEK_SET);
    fwrite(&seq_block, sizeof(seq_block), 1, ftm_file);
    fseek(ftm_file, block_end_addr, SEEK_SET);
}

void FTM_FILE::read_sequences_data() {
    sequences.clear();
    sequences.resize(FTM_SEQUENCE_TYPES);
    if (seq_block.sequ_num == 0) {
        return;
    }
    std::vector<std::vector<uint32_t>> index_map(seq_block.sequ_num, std::vector<uint32_t>(2, 0));
    DBG_PRINTF("\nREADING SEQUENCES, COUNT=%ld\n", seq_block.sequ_num);
    for (int i = 0; i < seq_block.sequ_num; i++) {
        sequences_t sequ_tmp;
        DBG_PRINTF("\nREADING SEQUENCES #%d...\n", i);
        read_u32(ftm_file, &sequ_tmp.index);
        read_u32(ftm_file, &sequ_tmp.type);
        read_u8(ftm_file, &sequ_tmp.length);
        read_u32(ftm_file, &sequ_tmp.loop);
        sequ_tmp.data.resize(sequ_tmp.length);
        fread(sequ_tmp.data.data(), 1, sequ_tmp.length, ftm_file);
        DBG_PRINTF("#%ld\n", sequ_tmp.index);
        DBG_PRINTF("TYPE: 0x%lX\n", sequ_tmp.type);
        DBG_PRINTF("LENGTH: %d\n", sequ_tmp.length);
        DBG_PRINTF("LOOP: %ld\n", sequ_tmp.loop);
        DBG_PRINTF("DATA:\n");
        for (int j = 0; j < sequ_tmp.length; j++) {
            DBG_PRINTF("%d ", sequ_tmp.data[j]);
        }
        if (sequ_tmp.type < FTM_SEQUENCE_TYPES) {
            if (sequ_tmp.index >= sequences[sequ_tmp.type].size()) {
                sequences[sequ_tmp.type].resize(sequ_tmp.index + 1);
            }
            sequences[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
            index_map[i][0] = sequ_tmp.type;
            index_map[i][1] = sequ_tmp.index;
        }
        DBG_PRINTF("\n");
    }
    DBG_PRINTF("\nREADING SEQU SETTING...\n");
    for (int i = 0; i < seq_block.sequ_num; i++) {
        DBG_PRINTF("\n#%d: %ld, %ld\n", i, index_map[i][0], index_map[i][1]);
        fread(&sequences[index_map[i][0]][index_map[i][1]].release, sizeof(uint32_t), 1, ftm_file);
        DBG_PRINTF("RELEASE: %ld\n", sequences[index_map[i][0]][index_map[i][1]].release);
        fread(&sequences[index_map[i][0]][index_map[i][1]].setting, sizeof(uint32_t), 1, ftm_file);
        DBG_PRINTF("SETTING: 0x%lX\n", sequences[index_map[i][0]][index_map[i][1]].setting);
    }
    DBG_PRINTF("SECCESS.\n");
    if (1) { //getchar() == '1') {
        DBG_PRINTF("\nVOLUME:\n");
        for (uint32_t i = 0; i < sequences[0].size(); i++) {
            DBG_PRINTF("#%ld: %d %ld %ld -> ", sequences[0][i].index, sequences[0][i].length, sequences[0][i].loop, sequences[0][i].release);
            for (int j = 0; j < sequences[0][i].length; j++) {
                DBG_PRINTF("%d ", sequences[0][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nARPEGGIO:\n");
        for (uint32_t i = 0; i < sequences[1].size(); i++) {
            DBG_PRINTF("#%ld: %d %ld %ld -> ", sequences[1][i].index, sequences[1][i].length, sequences[1][i].loop, sequences[1][i].release);
            for (int j = 0; j < sequences[1][i].length; j++) {
                DBG_PRINTF("%d ", sequences[1][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nPITCH:\n");
        for (uint32_t i = 0; i < sequences[2].size(); i++) {
            DBG_PRINTF("#%ld: %d %ld %ld -> ", sequences[2][i].index, sequences[2][i].length, sequences[2][i].loop, sequences[2][i].release);
            for (int j = 0; j < sequences[2][i].length; j++) {
                DBG_PRINTF("%d ", sequences[2][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nHI-PITCH:\n");
        for (uint32_t i = 0; i < sequences[3].size(); i++) {
            DBG_PRINTF("#%ld: %d %ld %ld -> ", sequences[3][i].index, sequences[3][i].length, sequences[3][i].loop, sequences[3][i].release);
            for (int j = 0; j < sequences[3][i].length; j++) {
                DBG_PRINTF("%d ", sequences[3][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nDUTY:\n");
        for (uint32_t i = 0; i < sequences[4].size(); i++) {
            DBG_PRINTF("#%ld: %d %ld %ld -> ", sequences[4][i].index, sequences[4][i].length, sequences[4][i].loop, sequences[4][i].release);
            for (int j = 0; j < sequences[4][i].length; j++) {
                DBG_PRINTF("%d ", sequences[4][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
    }
}

void FTM_FILE::read_frame_block() {
    fread(&fr_block, 1, sizeof(FRAME_BLOCK), ftm_file);
    DBG_PRINTF("\nFRAME HEADER: %s\n", fr_block.id);
    DBG_PRINTF("VERSION: %ld\n", fr_block.version);
    DBG_PRINTF("SIZE: %ld\n", fr_block.size);
    DBG_PRINTF("NUM_FRAMES: %ld\n", fr_block.frame_num);
    DBG_PRINTF("PATTERN_LENGTH: %ld\n", fr_block.pat_length);
    DBG_PRINTF("SPEED: %ld\n", fr_block.speed);
    DBG_PRINTF("TEMPO: %ld\n", fr_block.tempo);
}

void FTM_FILE::read_frame_data() {
    frames.clear();
    DBG_PRINTF("\nREADING FRAMES, COUNT=%ld\n", fr_block.frame_num);
    for (int i = 0; i < fr_block.frame_num; i++) {
        std::vector<uint8_t> fr_tmp;
        fr_tmp.resize(pr_block.channel);
        fread(fr_tmp.data(), 1, pr_block.channel, ftm_file);
        frames.push_back(fr_tmp);
        DBG_PRINTF("#%02X: ", i);
        for (int c = 0; c < pr_block.channel; c++) {
            DBG_PRINTF("%02X ", frames[i][c]);
        }
        DBG_PRINTF("\n");
    }
}

void FTM_FILE::write_frame() {
    fr_block.size = (fr_block.frame_num * pr_block.channel) + 16;
    fwrite(&fr_block, sizeof(FRAME_BLOCK), 1, ftm_file);

    for (int f = 0; f < fr_block.frame_num; f++) {
        fwrite(frames[f].data(), 1, pr_block.channel, ftm_file);
        // printf("FTELL: %ld\n", ftell(ftm_file));
    }
}

void FTM_FILE::read_pattern_block() {
    fread(&pt_block, 1, sizeof(PATTERN_BLOCK), ftm_file);
    DBG_PRINTF("\nPATTERN HEADER: %s\n", pt_block.id);
    DBG_PRINTF("VERSION: %ld\n", pt_block.version);
    DBG_PRINTF("SIZE: %ld\n", pt_block.size);
}

void FTM_FILE::read_pattern_data() {
    patterns.clear();
    unpack_pt.clear();
    patterns.resize(pr_block.channel);
    unpack_pt.resize(pr_block.channel);
    size_t pt_end = ftell(ftm_file) + pt_block.size;
    int count = 0;
    while (ftell(ftm_file) < pt_end) {
        pattern_t pt_tmp;
        fread(&pt_tmp, 4, 4, ftm_file);
        DBG_PRINTF("\n#%d\n", count);
        DBG_PRINTF("TRACK: %ld\n", pt_tmp.track);
        DBG_PRINTF("CHANNEL: %ld\n", pt_tmp.channel);
        DBG_PRINTF("INDEX: %ld\n", pt_tmp.index);
        DBG_PRINTF("ITEMS: %ld\n", pt_tmp.items);
        pt_tmp.item.resize(pt_tmp.items);
        int item_size = 10 + 2 * he_block.ch_fx[pt_tmp.channel];
        for (int i = 0; i < pt_tmp.items; i++) {
            fread(&pt_tmp.item[i], 1, item_size, ftm_file);
        }
        if (pt_tmp.channel >= pr_block.channel) {
            continue;
        }
        if (pt_tmp.index >= patterns[pt_tmp.channel].size()) {
            patterns[pt_tmp.channel].resize(pt_tmp.index + 1);
            patterns[pt_tmp.channel][pt_tmp.index] = pt_tmp;
        } else {
            patterns[pt_tmp.channel][pt_tmp.index] = pt_tmp;
        }
        // DBG_PRINTF("UNPACK...\n");
        if (pt_tmp.index >= unpack_pt[pt_tmp.channel].size()) {
            unpack_pt[pt_tmp.channel].resize(pt_tmp.index + 1);
        }
        unpack_pt[pt_tmp.channel][pt_tmp.index].resize(fr_block.pat_length);
        for (int y = 0; y < pt_tmp.items; y++) {
            if (pt_tmp.item[y].row >= fr_block.pat_length) continue;
            unpack_pt[pt_tmp.channel][pt_tmp.index][pt_tmp.item[y].row].note = pt_tmp.item[y].note;
            unpack_pt[pt_tmp.channel][pt_tmp.index][pt_tmp.item[y].row].octave = pt_tmp.item[y].octave;
            unpack_pt[pt_tmp.channel][pt_tmp.index][pt_tmp.item[y].row].instrument = pt_tmp.item[y].instrument;
            unpack_pt[pt_tmp.channel][pt_tmp.index][pt_tmp.item[y].row].volume = pt_tmp.item[y].volume;
            memcpy(unpack_pt[pt_tmp.channel][pt_tmp.index][pt_tmp.item[y].row].fxdata, pt_tmp.item[y].fxdata, 8);
        }
        count++;
        DBG_PRINTF("SECCESS.\n");
    }
    pattern_num = count;
}

void FTM_FILE::write_pattern() {
    size_t block_start_addr = ftell(ftm_file);
    fwrite(&pt_block, sizeof(PATTERN_BLOCK), 1, ftm_file);
    
    patterns.clear();
    patterns.resize(pr_block.channel);

    for (int c = 0; c < unpack_pt.size(); c++) {
        for (int i = 0; i < unpack_pt[c].size(); i++) {
            pattern_t pattern_tmp;
            pattern_tmp.track = 0;
            pattern_tmp.items = 0;
            pattern_tmp.channel = c;
            pattern_tmp.index = i;
            for (int r = 0; r < unpack_pt[c][i].size(); r++) {
                unpk_item_t pt_tmp = unpack_pt[c][i][r];
                if ((pt_tmp.note != NO_NOTE) ||
                    (pt_tmp.instrument != NO_INST) ||
                    (pt_tmp.volume != NO_VOL) ||
                    (pt_tmp.fxdata[0].fx_cmd) ||
                    (pt_tmp.fxdata[1].fx_cmd) ||
                    (pt_tmp.fxdata[2].fx_cmd) ||
                    (pt_tmp.fxdata[3].fx_cmd)) {
                    ft_item_t ft_tmp;
                    ft_tmp.row = r;
                    ft_tmp.note = pt_tmp.note;
                    ft_tmp.octave = pt_tmp.octave;
                    ft_tmp.instrument = pt_tmp.instrument;
                    ft_tmp.volume = pt_tmp.volume;
                    memcpy(ft_tmp.fxdata, pt_tmp.fxdata, 8);
                    pattern_tmp.item.push_back(ft_tmp);
                    pattern_tmp.items++;
                }
            }
            if (pattern_tmp.items) {
                patterns[c].push_back(pattern_tmp);
            }
        }
    }

    size_t tol_size = 0;

    for (int c = 0; c < patterns.size(); c++) {
        int item_size = 10 + 2 * he_block.ch_fx[c];
        for (int i = 0; i < patterns[c].size(); i++) {
            fwrite(&patterns[c][i], 4, 4, ftm_file);
            tol_size += 4 * 4;
            for (int item = 0; item < patterns[c][i].item.size(); item++) {
                fwrite(&patterns[c][i].item[item], item_size, 1, ftm_file);
                tol_size += item_size;
            }
        }
    }

    size_t block_end_addr = ftell(ftm_file);

    pt_block.size = tol_size;

    fseek(ftm_file, block_start_addr, SEEK_SET);
    fwrite(&pt_block, sizeof(PATTERN_BLOCK), 1, ftm_file);
    fseek(ftm_file, block_end_addr, SEEK_SET);
}

unpk_item_t FTM_FILE::get_pt_item(uint8_t c, uint8_t i, uint32_t r) {
    if (c >= unpack_pt.size() || i >= unpack_pt[c].size() || r >= unpack_pt[c][i].size()) {
        unpk_item_t pt_tmp;
        return pt_tmp;
    }
    return unpack_pt[c][i][r];
}

void FTM_FILE::set_pt_item(uint8_t c, uint8_t i, uint32_t r, unpk_item_t item) {
    if (c >= unpack_pt.size()) {
        unpack_pt.resize(c + 1);
    }
    if (i >= unpack_pt[c].size()) {
        unpack_pt[c].resize(i + 1);
    }
    if (r >= unpack_pt[c][i].size()) {
        unpack_pt[c][i].resize(r + 1);
    }
    unpack_pt[c][i][r] = item;
}

void FTM_FILE::read_dpcm_block() {
    dpcm_block = DPCM_SAMPLE_BLOCK();
    dpcm_block.size = 0;
    dpcm_block.sample_num = 0;
    size_t read = fread(dpcm_block.id, 1, 16, ftm_file);
    if (read < 16 || strncmp(dpcm_block.id, "DPCM SAMPLES", 16)) {
        if (read > 0) {
            fseek(ftm_file, -(long)read, SEEK_CUR);
        }
        dpcm_block = DPCM_SAMPLE_BLOCK();
        dpcm_block.size = 0;
        dpcm_block.sample_num = 0;
        DBG_PRINTF("NO DPCM SAMPLE BLOCK\n");
        return;
    }
    DBG_PRINTF("DPCM HEADER: %s\n", dpcm_block.id);
    fread(&dpcm_block.version, 4, 1, ftm_file);
    DBG_PRINTF("VERSION: %ld\n", dpcm_block.version);
    fread(&dpcm_block.size, 4, 1, ftm_file);
    DBG_PRINTF("SIZE: %ld\n", dpcm_block.size);
    dpcm_block.sample_num = 0;
    if (dpcm_block.size > 0) {
        fread(&dpcm_block.sample_num, 1, 1, ftm_file);
    }
    DBG_PRINTF("SAMPLE_NUM: %d\n", dpcm_block.sample_num);
}

void FTM_FILE::read_dpcm_data() {
    dpcm_samples.clear();
    if (dpcm_block.size <= 1 || dpcm_block.sample_num == 0) {
        DBG_PRINTF("NO DPCM SAMPLE\n");
        return;
    }
    for (int i = 0; i < dpcm_block.sample_num; i++) {
        uint8_t index;
        fread(&index, 1, 1, ftm_file);
        if (index >= dpcm_samples.size()) {
            dpcm_samples.resize(index + 1);
        }
        dpcm_samples[index].index = index;
        DBG_PRINTF("\n#%d\n", dpcm_samples[index].index);
        fread(&dpcm_samples[index].name_len, 4, 1, ftm_file);
        uint32_t copy_len = dpcm_samples[index].name_len < sizeof(dpcm_samples[index].name) - 1 ? dpcm_samples[index].name_len : sizeof(dpcm_samples[index].name) - 1;
        memset(dpcm_samples[index].name, 0, sizeof(dpcm_samples[index].name));
        fread(dpcm_samples[index].name, 1, copy_len, ftm_file);
        if (dpcm_samples[index].name_len > copy_len) {
            fseek(ftm_file, dpcm_samples[index].name_len - copy_len, SEEK_CUR);
        }
        dpcm_samples[index].name_len = copy_len;
        DBG_PRINTF("NAME(SIZE=%ld): %s\n", dpcm_samples[index].name_len, dpcm_samples[index].name);
        fread(&dpcm_samples[index].sample_size_byte, 4, 1, ftm_file);
        dpcm_samples[index].dpcm_data.resize(dpcm_samples[index].sample_size_byte);
        // DBG_PRINTF("FTELL: %d\n", ftell(ftm_file));
        fread(dpcm_samples[index].dpcm_data.data(), 1, dpcm_samples[index].sample_size_byte, ftm_file);
        DBG_PRINTF("SAMPLE SIZE(byte) = %ld\n", dpcm_samples[index].sample_size_byte);
        DBG_PRINTF("DECODE...\n");
        dpcm_samples[index].pcm_data.resize(dpcm_samples[index].sample_size_byte * 8);
        decode_dpcm(dpcm_samples[index].dpcm_data.data(), dpcm_samples[index].sample_size_byte, dpcm_samples[index].pcm_data.data());
        // DBG_PRINTF("FTELL: %d\n", ftell(ftm_file));
        DBG_PRINTF("SECCESS.\n");
    }
}

void FTM_FILE::write_dpcm() {
    size_t block_start_addr = ftell(ftm_file);
    uint8_t sample_count = 0;
    for (size_t i = 0; i < dpcm_samples.size(); i++) {
        if (dpcm_samples[i].sample_size_byte && !dpcm_samples[i].dpcm_data.empty()) {
            sample_count++;
        }
    }
    dpcm_block.sample_num = sample_count;
    dpcm_block.size = sample_count ? 1 : 0;
    if (sample_count == 0) {
        return;
    }
    fwrite(dpcm_block.id, 1, 16, ftm_file);
    fwrite(&dpcm_block.version, 4, 1, ftm_file);
    fwrite(&dpcm_block.size, 4, 1, ftm_file);
    fwrite(&dpcm_block.sample_num, 1, 1, ftm_file);
    
    size_t tol_size = 1;
    for (int i = 0; i < dpcm_samples.size(); i++) {
        dpcm_samples[i].index = i;
        if (dpcm_samples[i].sample_size_byte && !dpcm_samples[i].dpcm_data.empty()) {
            dpcm_samples[i].name_len = static_cast<uint32_t>(bounded_strlen(dpcm_samples[i].name, sizeof(dpcm_samples[i].name)));
            uint32_t write_size = normalized_dpcm_size(dpcm_samples[i].sample_size_byte);
            fwrite(&dpcm_samples[i].index, 1, 1, ftm_file);
            fwrite(&dpcm_samples[i].name_len, 4, 1, ftm_file);
            fwrite(dpcm_samples[i].name, 1, dpcm_samples[i].name_len, ftm_file);
            fwrite(&write_size, 4, 1, ftm_file);
            uint32_t direct_size = write_size < dpcm_samples[i].dpcm_data.size() ? write_size : dpcm_samples[i].dpcm_data.size();
            fwrite(dpcm_samples[i].dpcm_data.data(), 1, direct_size, ftm_file);
            for (uint32_t pad = direct_size; pad < write_size; ++pad) {
                write_u8(ftm_file, 0xAA);
            }
            tol_size += 1 + 4 + dpcm_samples[i].name_len + 4 + write_size;
        }
    }
    dpcm_block.size = static_cast<uint32_t>(tol_size);

    size_t block_end_addr = ftell(ftm_file);

    fseek(ftm_file, block_start_addr, SEEK_SET);
    fwrite(dpcm_block.id, 1, 16, ftm_file);
    fwrite(&dpcm_block.version, 4, 1, ftm_file);
    fwrite(&dpcm_block.size, 4, 1, ftm_file);
    fwrite(&dpcm_block.sample_num, 1, 1, ftm_file);
    fseek(ftm_file, block_end_addr, SEEK_SET);
}

void FTM_FILE::print_frame_data(int index) {
    DBG_PRINTF("FRAME #%02X: ", index);
    for (int c = 0; c < pr_block.channel; c++) {
        DBG_PRINTF("%02X ", frames[index][c]);
    }
    DBG_PRINTF("\n");
    for (int y = 0; y < fr_block.pat_length; y++) {
        DBG_PRINTF("#%02X| ", y);
        for (int x = 0; x < pr_block.channel; x++) {
            unpk_item_t pt_tmp = get_pt_item(x, frames[index][x], y);
            if (pt_tmp.note != NO_NOTE) {
                DBG_PRINTF("%s%d ", note2str[pt_tmp.note], pt_tmp.octave);
            } else {
                DBG_PRINTF("... ");
            }
            if (pt_tmp.instrument != NO_INST) {
                DBG_PRINTF("%02X ", pt_tmp.instrument);
            } else {
                DBG_PRINTF(".. ");
            }
            if (pt_tmp.volume != NO_VOL) {
                DBG_PRINTF("%X ", pt_tmp.volume);
            } else {
                DBG_PRINTF(". ");
            }
            for (uint8_t fx = 0; fx < he_block.ch_fx[x] + 1; fx++) {
                if (pt_tmp.fxdata[fx].fx_cmd) {
                    DBG_PRINTF("%02X%02X ", pt_tmp.fxdata[fx].fx_cmd, pt_tmp.fxdata[fx].fx_var);
                } else {
                    DBG_PRINTF(".... ");
                }
            }
            DBG_PRINTF("| ");
        }
        DBG_PRINTF("\n");
    }
}

int FTM_FILE::read_ftm_all() {
    if (ftm_file == NULL) {
        DBG_PRINTF("No files were opened and could not be read.\n");
        return -1;
    }

    if (read_param_block()) {
        fclose(ftm_file);
        return NO_SUPPORT_EXTCHIP;
    }
    
    read_info_block();

    if (read_header_block()) {
        fclose(ftm_file);
        return NO_SUPPORT_MULTITRACK;
    }

    read_instrument_block();
    read_instrument_data();

    read_sequences_block();
    read_sequences_data();
    read_vrc6_sequences_block();
    read_vrc6_sequences_data();

    read_frame_block();
    read_frame_data();

    read_pattern_block();
    read_pattern_data();

    read_dpcm_block();
    read_dpcm_data();

    init_sequence_block(n163_seq_block, "SEQUENCES_N163");
    n163_sequences.clear();
    n163_sequences.resize(FTM_SEQUENCE_TYPES);

    while (true) {
        SEQUENCES_BLOCK extra_block;
        size_t read_size = fread(&extra_block, 1, sizeof(extra_block), ftm_file);
        if (read_size < sizeof(extra_block)) {
            break;
        }
        if (!strncmp(extra_block.id, "SEQUENCES_VRC6", 16)) {
            vrc6_seq_block = extra_block;
            read_vrc6_sequences_data();
        } else if (!strncmp(extra_block.id, "SEQUENCES_N163", 16)) {
            n163_seq_block = extra_block;
            read_n163_sequences_data();
        } else {
            fseek(ftm_file, extra_block.size, SEEK_CUR);
        }
    }

    fclose(ftm_file);
    return 0;
}

uint8_t FTM_FILE::ch_fx_count(int n) {
    return he_block.ch_fx[n] + 1;
}

uint8_t FTM_FILE::get_frame_map(int f, int c) {
    if (f >= frames.size() || c >= frames[f].size()) {
        return 0;
    }
    return frames[f][c];
}

void FTM_FILE::set_frame_map(int f, int c, int n) {
    if (f >= frames.size() || c >= frames[f].size()) {
        return;
    }
    frames[f][c] = n;
}

void FTM_FILE::set_frame_plus1(int f, int c) {
    if (f >= frames.size() || c >= frames[f].size()) {
        return;
    }
    frames[f][c]++;
}

void FTM_FILE::set_frame_minus1(int f, int c) {
    if (f >= frames.size() || c >= frames[f].size()) {
        return;
    }
    frames[f][c]--;
}

instrument_t* FTM_FILE::get_inst(int n) {
    if (n >= instrument.size()) return NULL;
    return &instrument[n];
}

int8_t FTM_FILE::get_sequ_data(int type, int index, int seq_index) {
    if (type >= sequences.size()) return 0;
    if (index >= sequences[type].size()) return 0;
    if (seq_index >= sequences[type][index].data.size()) return 0;
    return sequences[type][index].data[seq_index];
}

sequences_t* FTM_FILE::get_sequ(int type, int index) {
    if (type >= sequences.size()) return NULL;
    if (index >= sequences[type].size()) return NULL;
    return &sequences[type][index];
}

int8_t FTM_FILE::get_inst_sequ_data(instrument_t *inst, int type, int index, int seq_index) {
    SequenceBank *bank = &sequences;
    if (inst != NULL && inst->type == INST_VRC6) {
        bank = &vrc6_sequences;
    } else if (inst != NULL && inst->type == INST_N163) {
        bank = &n163_sequences;
    }
    if (type >= bank->size()) return 0;
    if (index >= (*bank)[type].size()) return 0;
    if (seq_index >= (*bank)[type][index].data.size()) return 0;
    return (*bank)[type][index].data[seq_index];
}

sequences_t* FTM_FILE::get_inst_sequ(instrument_t *inst, int type, int index) {
    SequenceBank *bank = &sequences;
    if (inst != NULL && inst->type == INST_VRC6) {
        bank = &vrc6_sequences;
    } else if (inst != NULL && inst->type == INST_N163) {
        bank = &n163_sequences;
    }
    if (type >= bank->size()) return NULL;
    if (index >= (*bank)[type].size()) return NULL;
    return &(*bank)[type][index];
}

uint8_t FTM_FILE::get_sequ_len(int type, int index) {
    if (type >= sequences.size()) return 0;
    if (index >= sequences[type].size()) return 0;
    return sequences[type][index].length;
}

void FTM_FILE::resize_sequ_len(int type, int index, int n) {
    if (type >= sequences.size()) return;
    if (index >= sequences[type].size()) return;
    sequences[type][index].data.resize(n);
    sequences[type][index].length = sequences[type][index].data.size();
    sequences[type][index].type = type;
}

void FTM_FILE::resize_inst_sequ_len(instrument_t *inst, int type, int index, int n) {
    SequenceBank *bank = &sequences;
    if (inst != NULL && inst->type == INST_VRC6) {
        bank = &vrc6_sequences;
    } else if (inst != NULL && inst->type == INST_N163) {
        bank = &n163_sequences;
    }
    if (type >= bank->size()) return;
    if (index >= (*bank)[type].size()) return;
    (*bank)[type][index].data.resize(n);
    (*bank)[type][index].length = (*bank)[type][index].data.size();
    (*bank)[type][index].type = type;
}

void FTM_FILE::resize_sequ(int type, int size) {
    if (type >= sequences.size()) return;
    sequences[type].resize(size);
    sequences[type][size - 1].type = type;
}

void FTM_FILE::resize_inst_sequ(instrument_t *inst, int type, int size) {
    SequenceBank *bank = &sequences;
    if (inst != NULL && inst->type == INST_VRC6) {
        bank = &vrc6_sequences;
    } else if (inst != NULL && inst->type == INST_N163) {
        bank = &n163_sequences;
    }
    if (type >= bank->size()) return;
    (*bank)[type].resize(size);
    (*bank)[type][size - 1].type = type;
}

void FTM_FILE::set_sequ_loop(int type, int index, uint32_t n) {
    if (type >= sequences.size()) return;
    if (index >= sequences[type].size()) return;
    sequences[type][index].loop = n;
}

uint8_t FTM_FILE::get_sequ_loop(int type, int index) {
    if (type >= sequences.size()) return 0;
    if (index >= sequences[type].size()) return 0;
    return sequences[type][index].loop;
}

void FTM_FILE::set_sequ_release(int type, int index, uint32_t n) {
    if (type >= sequences.size()) return;
    if (index >= sequences[type].size()) return;
    sequences[type][index].release = n;
}

uint8_t FTM_FILE::get_sequ_release(int type, int index) {
    if (type >= sequences.size()) return 0;
    if (index >= sequences[type].size()) return 0;
    return sequences[type][index].release;
}

void FTM_FILE::new_frame() {
    uint8_t next_pt = frames.size();
    std::vector<uint8_t> next_fr(pr_block.channel, next_pt);
    frames.push_back(next_fr);
    resize_channel_storage(pr_block.channel);
    fr_block.frame_num = frames.size();
}

void FTM_FILE::insert_new_frame(int n) {
    uint8_t next_pt = frames.size();
    std::vector<uint8_t> next_fr(pr_block.channel, next_pt);
    frames.insert(frames.begin() + n, next_fr);
    resize_channel_storage(pr_block.channel);
    fr_block.frame_num = frames.size();
}

void FTM_FILE::remove_frame(int n) {
    frames.erase(frames.begin() + n);
    fr_block.frame_num = frames.size();
}

void FTM_FILE::moveup_frame(int n) {
    if (n > 0 && n < frames.size()) {
        std::swap(frames[n], frames[n - 1]);
    }
}

void FTM_FILE::movedown_frame(int n) {
    if (n < frames.size() - 1) {
        std::swap(frames[n], frames[n + 1]);
    }
}

FTM_FILE ftm;
