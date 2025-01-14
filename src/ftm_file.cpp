#include "ftm_file.h"
extern int errno;

FTM_FILE::FTM_FILE() {
    patterns.resize(pr_block.channel);
    unpack_pt.resize(pr_block.channel);
    for (int c = 0; c < pr_block.channel; c++) {
        unpack_pt[c].resize(1);
        unpack_pt[c][0].resize(fr_block.pat_length);
    }
}

int FTM_FILE::open_ftm(const char *filename) {
    ftm_file = fopen(filename, "rb+");
    if (ftm_file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return -1;
    }
    fread(&header, sizeof(header), 1, ftm_file);
    if (strncmp(header.id, "FamiTracker Module", 18)) {
        printf("HEADER: %.16s\n", header.id);
        printf("This is not a valid FTM file!\n");
        memset(&header, 0, sizeof(header));
        fclose(ftm_file);
        return -2;
    };
    if (header.version != 0x0440) {
        printf("Only FTM file version 0x0440 is supported\nVERSION: 0x%X\n", header.version);
        memset(&header, 0, sizeof(header));
        fclose(ftm_file);
        return -3;
    }
    printf("\nOpen %.18s\n", header.id);
    printf("VERSION: 0x%X\n", header.version);
    return 0;
}

void FTM_FILE::read_param_block() {
    fread(&pr_block, 1, sizeof(pr_block), ftm_file);

    printf("\nPARAMS HEADER: %s\n", pr_block.id);
    printf("VERSION: %d\n", pr_block.version);
    printf("SIZE: %d\n", pr_block.size);
    printf("EXTRA_CHIP: 0x%X\n", pr_block.ext_chip);
    printf("CHANNELS: %d\n", pr_block.channel);
    printf("MACHINE: 0x%X\n", pr_block.machine);
    printf("E_SPEED: %d\n", pr_block.e_speed);
    printf("V_STYLE: %d\n", pr_block.v_style);
    printf("HIGHLINE1: %d\n", pr_block.hl1);
    printf("HIGHLINE2: %d\n", pr_block.hl2);
}

void FTM_FILE::read_info_block() {
    fread(&nf_block, 1, sizeof(nf_block), ftm_file);

    printf("\nINFO HEADER: %s\n", nf_block.id);
    printf("VERSION: %d\n", nf_block.version);
    printf("SIZE: %d\n", nf_block.size);
    printf("TITLE: %s\n", nf_block.title);
    printf("AUTHOR: %s\n", nf_block.author);
    printf("COPYRIGHT: %s\n", nf_block.copyright);
}

void FTM_FILE::read_header_block() {
    fread(&he_block, 1, 25, ftm_file);

    fread(&he_block.name, 1, he_block.size - (pr_block.channel * 2) - 1, ftm_file);
    if (he_block.track_num > 1) {
        printf("Multi-track FTM files are not supported\n");
        return;
    }

    for (int i = 0; i < pr_block.channel; i++) {
        fread(&he_block.ch_id[i], 1, 1, ftm_file);
        fread(&he_block.ch_fx[i], 1, 1, ftm_file);
    }

    printf("\nMETADATA HEADER: %s\n", he_block.id);
    printf("VERSION: %d\n", he_block.version);
    printf("SIZE: %d\n", he_block.size);
    printf("TRACK: %d\n", he_block.track_num + 1);
    printf("NAME: %s\n", he_block.name);
    for (uint8_t i = 0; i < pr_block.channel; i++) {
        printf("CH%d: EX_FX*%d  ", he_block.ch_id[i], he_block.ch_fx[i]);
    }

    printf("\n");
}

void FTM_FILE::read_instrument_block() {
    fread(&inst_block, 1, sizeof(inst_block), ftm_file);
    printf("\nINSTRUMENT HEADER: %s\n", inst_block.id);
    printf("VERSION: %d\n", inst_block.version);
    printf("SIZE: %d\n", inst_block.size);
    printf("NUM_INST: 0x%X\n", inst_block.inst_num);
}

void FTM_FILE::read_instrument_data() {
    instrument.clear();
    printf("\nREADING INSTRUMENT, COUNT=%d\n", inst_block.inst_num);
    for (int i = 0; i < inst_block.inst_num; i++) {
        instrument_t inst_tmp;
        printf("\nREADING INSTRUMENT #%02X...\n", i);
        fread(&inst_tmp, sizeof(instrument_t) - 64, 1, ftm_file);
        fread(&inst_tmp.name, inst_tmp.name_len, 1, ftm_file);
        inst_tmp.name[inst_tmp.name_len] = '\0';
        if (inst_tmp.index >= instrument.size()) {
            instrument.resize(inst_tmp.index + 1);
            instrument[inst_tmp.index] = inst_tmp;
        } else {
            instrument[inst_tmp.index] = inst_tmp;
        }
        printf("%02X->NAME: %s\n", inst_tmp.index, inst_tmp.name);
        printf("TYPE: 0x%X\n", inst_tmp.type);
        printf("SEQU COUNT: %d\n", inst_tmp.seq_count);
        printf("SEQU DATA:\n");
        printf("VOL: %d %d\n", inst_tmp.seq_index[0].enable, inst_tmp.seq_index[0].seq_index);
        printf("ARP: %d %d\n", inst_tmp.seq_index[1].enable, inst_tmp.seq_index[1].seq_index);
        printf("PIT: %d %d\n", inst_tmp.seq_index[2].enable, inst_tmp.seq_index[2].seq_index);
        printf("HIP: %d %d\n", inst_tmp.seq_index[3].enable, inst_tmp.seq_index[3].seq_index);
        printf("DTY: %d %d\n", inst_tmp.seq_index[4].enable, inst_tmp.seq_index[4].seq_index);
        printf("\n");
    }
}

void FTM_FILE::read_sequences_block() {
    fread(&seq_block, 1, sizeof(seq_block), ftm_file);
    printf("\nSEQUENCES HEADER: %s\n", seq_block.id);
    printf("VERSION: %d\n", seq_block.version);
    printf("SIZE: %d\n", seq_block.size);
    printf("NUM_SEQU: %d\n", seq_block.sequ_num);
}

void FTM_FILE::read_sequences_data() {
    sequences.resize(5);
    uint32_t index_map[seq_block.sequ_num][2];
    printf("\nREADING SEQUENCES, COUNT=%d\n", seq_block.sequ_num);
    for (int i = 0; i < seq_block.sequ_num; i++) {
        sequences_t sequ_tmp;
        printf("\nREADING SEQUENCES #%d...\n", i);
        fread(&sequ_tmp, 13, 1, ftm_file);
        sequ_tmp.data.resize(sequ_tmp.length);
        fread(sequ_tmp.data.data(), sequ_tmp.length, 1, ftm_file);
        printf("#%d\n", sequ_tmp.index);
        printf("TYPE: 0x%X\n", sequ_tmp.type);
        printf("LENGTH: %d\n", sequ_tmp.length);
        printf("LOOP: %d\n", sequ_tmp.loop);
        printf("DATA:\n");
        for (int j = 0; j < sequ_tmp.length; j++) {
            printf("%d ", sequ_tmp.data[j]);
        }
        if (sequ_tmp.index >= sequences[sequ_tmp.type].size()) {
            sequences[sequ_tmp.type].resize(sequ_tmp.index + 1);
            sequences[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
        } else {
            sequences[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
        }
        index_map[i][0] = sequ_tmp.type;
        index_map[i][1] = sequ_tmp.index;
        printf("\n");
    }
    printf("\nREADING SEQU SETTING...\n");
    for (int i = 0; i < seq_block.sequ_num; i++) {
        printf("\n#%d: %d, %d\n", i, index_map[i][0], index_map[i][1]);
        fread(&sequences[index_map[i][0]][index_map[i][1]].release, sizeof(uint32_t), 1, ftm_file);
        printf("RELEASE: %d\n", sequences[index_map[i][0]][index_map[i][1]].release);
        fread(&sequences[index_map[i][0]][index_map[i][1]].setting, sizeof(uint32_t), 1, ftm_file);
        printf("SETTING: 0x%X\n", sequences[index_map[i][0]][index_map[i][1]].setting);
    }
    printf("SECCESS.\n");
    if (1) { //getchar() == '1') {
        printf("\nVOLUME:\n");
        for (uint32_t i = 0; i < sequences[0].size(); i++) {
            printf("#%d: %d %d %d -> ", sequences[0][i].index, sequences[0][i].length, sequences[0][i].loop, sequences[0][i].release);
            for (int j = 0; j < sequences[0][i].length; j++) {
                printf("%d ", sequences[0][i].data[j]);
            }
            printf("\n");
        }
        printf("\nARPEGGIO:\n");
        for (uint32_t i = 0; i < sequences[1].size(); i++) {
            printf("#%d: %d %d %d -> ", sequences[1][i].index, sequences[1][i].length, sequences[1][i].loop, sequences[1][i].release);
            for (int j = 0; j < sequences[1][i].length; j++) {
                printf("%d ", sequences[1][i].data[j]);
            }
            printf("\n");
        }
        printf("\nPITCH:\n");
        for (uint32_t i = 0; i < sequences[2].size(); i++) {
            printf("#%d: %d %d %d -> ", sequences[2][i].index, sequences[2][i].length, sequences[2][i].loop, sequences[2][i].release);
            for (int j = 0; j < sequences[2][i].length; j++) {
                printf("%d ", sequences[2][i].data[j]);
            }
            printf("\n");
        }
        printf("\nHI-PITCH:\n");
        for (uint32_t i = 0; i < sequences[3].size(); i++) {
            printf("#%d: %d %d %d -> ", sequences[3][i].index, sequences[3][i].length, sequences[3][i].loop, sequences[3][i].release);
            for (int j = 0; j < sequences[3][i].length; j++) {
                printf("%d ", sequences[3][i].data[j]);
            }
            printf("\n");
        }
        printf("\nDUTY:\n");
        for (uint32_t i = 0; i < sequences[4].size(); i++) {
            printf("#%d: %d %d %d -> ", sequences[4][i].index, sequences[4][i].length, sequences[4][i].loop, sequences[4][i].release);
            for (int j = 0; j < sequences[4][i].length; j++) {
                printf("%d ", sequences[4][i].data[j]);
            }
            printf("\n");
        }
    }
}

void FTM_FILE::read_frame_block() {
    fread(&fr_block, sizeof(FRAME_BLOCK), 1, ftm_file);
    printf("\nFRAME HEADER: %s\n", fr_block.id);
    printf("VERSION: %d\n", fr_block.version);
    printf("SIZE: %d\n", fr_block.size);
    printf("NUM_FRAMES: %d\n", fr_block.frame_num);
    printf("PATTERN_LENGTH: %d\n", fr_block.pat_length);
    printf("SPEED: %d\n", fr_block.speed);
    printf("TEMPO: %d\n", fr_block.tempo);
}

void FTM_FILE::read_frame_data() {
    frames.clear();
    printf("\nREADING FRAMES, COUNT=%d\n", fr_block.frame_num);
    for (int i = 0; i < fr_block.frame_num; i++) {
        std::vector<uint8_t> fr_tmp;
        fr_tmp.resize(pr_block.channel);
        fread(fr_tmp.data(), pr_block.channel, 1, ftm_file);
        frames.push_back(fr_tmp);
        printf("#%02X: ", i);
        for (int c = 0; c < pr_block.channel; c++) {
            printf("%02X ", frames[i][c]);
        }
        printf("\n");
    }
}

void FTM_FILE::read_pattern_block() {
    fread(&pt_block, sizeof(PATTERN_BLOCK), 1, ftm_file);
    printf("\nPATTERN HEADER: %s\n", pt_block.id);
    printf("VERSION: %d\n", pt_block.version);
    printf("SIZE: %d\n", pt_block.size);
}

void FTM_FILE::read_pattern_data() {
    patterns.clear();
    patterns.resize(pr_block.channel);
    unpack_pt.resize(pr_block.channel);
    size_t pt_end = ftell(ftm_file) + pt_block.size;
    int count = 0;
    while (ftell(ftm_file) < pt_end) {
        pattern_t pt_tmp;
        fread(&pt_tmp, 4, 4, ftm_file);
        // printf("\n#%d\n", count);
        // printf("TRACK: %d\n", pt_tmp.track);
        // printf("CHANNEL: %d\n", pt_tmp.channel);
        // printf("INDEX: %d\n", pt_tmp.index);
        // printf("ITEMS: %d\n", pt_tmp.items);
        pt_tmp.item.resize(pt_tmp.items);
        int item_size = 10 + 2 * he_block.ch_fx[pt_tmp.channel];
        for (int i = 0; i < pt_tmp.items; i++) {
            fread(&pt_tmp.item[i], 1, item_size, ftm_file);
        }
        if (pt_tmp.index >= patterns[pt_tmp.channel].size()) {
            patterns[pt_tmp.channel].resize(pt_tmp.index + 1);
            patterns[pt_tmp.channel][pt_tmp.index] = pt_tmp;
        } else {
            patterns[pt_tmp.channel][pt_tmp.index] = pt_tmp;
        }
        // printf("UNPACK...\n");
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
        // printf("SECCESS.\n");
    }
    pattern_num = count;
}

unpk_item_t FTM_FILE::get_pt_item(uint8_t c, uint8_t i, uint32_t r) {
    if (i >= unpack_pt[c].size() || r >= unpack_pt[c][i].size()) {
        unpk_item_t pt_tmp;
        return pt_tmp;
    }
    return unpack_pt[c][i][r];
}

void FTM_FILE::print_frame_data(int index) {
    printf("FRAME #%02X: ", index);
    for (int c = 0; c < pr_block.channel; c++) {
        printf("%02X ", frames[index][c]);
    }
    printf("\n");
    for (int y = 0; y < fr_block.pat_length; y++) {
        printf("#%02X| ", y);
        for (int x = 0; x < pr_block.channel; x++) {
            unpk_item_t pt_tmp = get_pt_item(x, frames[index][x], y);
            if (pt_tmp.note != NO_NOTE) {
                printf("%s%d ", note2str[pt_tmp.note], pt_tmp.octave);
            } else {
                printf("... ");
            }
            if (pt_tmp.instrument != NO_INST) {
                printf("%02X ", pt_tmp.instrument);
            } else {
                printf(".. ");
            }
            if (pt_tmp.volume != NO_VOL) {
                printf("%X ", pt_tmp.volume);
            } else {
                printf(". ");
            }
            for (uint8_t fx = 0; fx < he_block.ch_fx[x] + 1; fx++) {
                if (pt_tmp.fxdata[fx].fx_cmd) {
                    printf("%02X%02X ", pt_tmp.fxdata[fx].fx_cmd, pt_tmp.fxdata[fx].fx_var);
                } else {
                    printf(".... ");
                }
            }
            printf("| ");
        }
        printf("\n");
    }
}

void FTM_FILE::read_dpcm_block() {
    fread(dpcm_block.id, 1, 16, ftm_file);
    printf("DPCM HEADER: %s\n", dpcm_block.id);
    fread(&dpcm_block.version, 4, 1, ftm_file);
    printf("VERSION: %d\n", dpcm_block.version);
    fread(&dpcm_block.size, 4, 1, ftm_file);
    printf("SIZE: %d\n", dpcm_block.size);
    fread(&dpcm_block.sample_num, 2, 1, ftm_file);
    printf("SAMPLE_NUM: %d\n", dpcm_block.sample_num);
}

void FTM_FILE::read_dpcm_data() {
    if (dpcm_block.size == 1) {
        printf("NO DPCM SAMPLE\n");
        return;
    }
    dpcm_samples.resize(dpcm_block.sample_num);
    printf("RESIZE DPCM_SAMPLES TO %d\n", dpcm_samples.size());
    for (int i = 0; i < dpcm_block.sample_num; i++) {
        printf("\n#%d\n", i);
        fread(&dpcm_samples[i].name_len, 4, 1, ftm_file);
        fread(dpcm_samples[i].name, 1, dpcm_samples[i].name_len, ftm_file);
        printf("NAME(SIZE=%d): %s\n", dpcm_samples[i].name_len, dpcm_samples[i].name);
        fread(&dpcm_samples[i].sample_size_byte, 4, 1, ftm_file);
        dpcm_samples[i].dpcm_data.resize(dpcm_samples[i].sample_size_byte);
        // printf("FTELL: %d\n", ftell(ftm_file));
        fread(dpcm_samples[i].dpcm_data.data(), 1, dpcm_samples[i].sample_size_byte, ftm_file);
        printf("SAMPLE SIZE(byte) = %d\n", dpcm_samples[i].sample_size_byte);
        printf("DECODE...\n");
        dpcm_samples[i].pcm_data.resize(dpcm_samples[i].sample_size_byte * 8);
        decode_dpcm(dpcm_samples[i].dpcm_data.data(), dpcm_samples[i].sample_size_byte, dpcm_samples[i].pcm_data.data());
        fseek(ftm_file, 1, SEEK_CUR);
        // printf("FTELL: %d\n", ftell(ftm_file));
        printf("SECCESS.\n");
    }
}

void FTM_FILE::read_ftm_all() {
    if (ftm_file == NULL) {
        printf("No files were opened and could not be read.\n");
        return;
    }

    read_param_block();
    read_info_block();
    read_header_block();

    read_instrument_block();
    read_instrument_data();

    read_sequences_block();
    read_sequences_data();

    read_frame_block();
    read_frame_data();

    read_pattern_block();
    read_pattern_data();

    read_dpcm_block();
    read_dpcm_data();
}

uint8_t FTM_FILE::ch_fx_count(int n) {
    return he_block.ch_fx[n] + 1;
}

FTM_FILE ftm;