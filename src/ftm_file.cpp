#include "ftm_file.h"
extern int errno;
extern bool _debug_print;

FTM_FILE::FTM_FILE() {
    new_ftm();
}

void FTM_FILE::new_ftm() {
    FTM_HEADER hd_new;
    PARAMS_BLOCK pr_new;
    INFO_BLOCK nf_new;
    HEADER_BLOCK he_new;
    INSTRUMENT_BLOCK inst_new;
    SEQUENCES_BLOCK seq_new;
    FRAME_BLOCK fr_new;
    PATTERN_BLOCK pt_new;
    DPCM_SAMPLE_BLOCK dpcm_new;

    header = hd_new;
    pr_block = pr_new;
    nf_block = nf_new;
    he_block = he_new;
    inst_block = inst_new;
    seq_block = seq_new;
    fr_block = fr_new;
    pt_block = pt_new;
    dpcm_block = dpcm_new;

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
    sequences.resize(5);
    create_new_inst();
}

void FTM_FILE::create_new_inst() {
    if (instrument.empty()) {
        instrument.emplace_back();
        instrument[0].index = 0;
        for (int i = 0; i < 5; ++i) {
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

    for (int i = 0; i < 5; ++i) {
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

int FTM_FILE::open_ftm(const char *filename) {
    ftm_file = fopen(filename, "rb");
    if (ftm_file == NULL) {
        perror("Error opening file");
        return FILE_OPEN_ERROR;
    }
    fread(&header, sizeof(header), 1, ftm_file);
    if (strncmp(header.id, "FamiTracker Module", 18)) {
        DBG_PRINTF("HEADER: %.16s\n", header.id);
        DBG_PRINTF("This is not a valid FTM file!\n");
        memset(&header, 0, sizeof(header));
        fclose(ftm_file);
        return FTM_NOT_VALID;
    };
    if (header.version != 0x0440) {
        DBG_PRINTF("Only FTM file version 0x0440 is supported\nVERSION: 0x%X\n", header.version);
        memset(&header, 0, sizeof(header));
        fclose(ftm_file);
        return NO_SUPPORT_VERSION;
    }
    DBG_PRINTF("\nOpen %.18s\n", header.id);
    DBG_PRINTF("VERSION: 0x%X\n", header.version);
    return 0;
}

int FTM_FILE::save_ftm(const char *filename) {
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

    fwrite("\0END", 1, 4, ftm_file);

    fclose(ftm_file);

    return 0;
}

void FTM_FILE::write_file_header() {
    fwrite(&header, sizeof(header), 1, ftm_file);
}

void FTM_FILE::write_param_block() {
    pr_block.size = 29;
    fwrite(&pr_block, 1, sizeof(pr_block), ftm_file);
}

int FTM_FILE::read_param_block() {
    fread(&pr_block, 1, sizeof(pr_block), ftm_file);

    if (pr_block.ext_chip != 0) {
        DBG_PRINTF("Multi-Chip FTM files are not supported\n");
        return NO_SUPPORT_EXTCHIP;
    }

    DBG_PRINTF("\nPARAMS HEADER: %s\n", pr_block.id);
    DBG_PRINTF("VERSION: %d\n", pr_block.version);
    DBG_PRINTF("SIZE: %d\n", pr_block.size);
    DBG_PRINTF("EXTRA_CHIP: 0x%X\n", pr_block.ext_chip);
    DBG_PRINTF("CHANNELS: %d\n", pr_block.channel);
    DBG_PRINTF("MACHINE: 0x%X\n", pr_block.machine);
    DBG_PRINTF("E_SPEED: %d\n", pr_block.e_speed);
    DBG_PRINTF("V_STYLE: %d\n", pr_block.v_style);
    DBG_PRINTF("HIGHLINE1: %d\n", pr_block.hl1);
    DBG_PRINTF("HIGHLINE2: %d\n", pr_block.hl2);
    return 0;
}

void FTM_FILE::read_info_block() {
    fread(&nf_block, 1, sizeof(nf_block), ftm_file);

    DBG_PRINTF("\nINFO HEADER: %s\n", nf_block.id);
    DBG_PRINTF("VERSION: %d\n", nf_block.version);
    DBG_PRINTF("SIZE: %d\n", nf_block.size);
    DBG_PRINTF("TITLE: %s\n", nf_block.title);
    DBG_PRINTF("AUTHOR: %s\n", nf_block.author);
    DBG_PRINTF("COPYRIGHT: %s\n", nf_block.copyright);
}

void FTM_FILE::write_info_block() {
    nf_block.size = 96;
    fwrite(&nf_block, 1, sizeof(nf_block), ftm_file);
}

int FTM_FILE::read_header_block() {
    fread(&he_block, 1, 25, ftm_file);

    fread(&he_block.name, 1, he_block.size - (pr_block.channel * 2) - 1, ftm_file);
    if (he_block.track_num > 0) {
        DBG_PRINTF("Multi-track FTM files are not supported\n");
        return NO_SUPPORT_MULTITRACK;
    }

    for (int i = 0; i < pr_block.channel; i++) {
        fread(&he_block.ch_id[i], 1, 1, ftm_file);
        fread(&he_block.ch_fx[i], 1, 1, ftm_file);
    }

    DBG_PRINTF("\nMETADATA HEADER: %s\n", he_block.id);
    DBG_PRINTF("VERSION: %d\n", he_block.version);
    DBG_PRINTF("SIZE: %d\n", he_block.size);
    DBG_PRINTF("TRACK: %d\n", he_block.track_num + 1);
    DBG_PRINTF("NAME: %s\n", he_block.name);
    for (uint8_t i = 0; i < pr_block.channel; i++) {
        DBG_PRINTF("CH%d: EX_FX*%d  ", he_block.ch_id[i], he_block.ch_fx[i]);
    }

    DBG_PRINTF("\n");
    return 0;
}

void FTM_FILE::write_header_block() {
    he_block.size = 20;
    fwrite(&he_block, 1, 25, ftm_file);
    strcpy(he_block.name, "_FAMI32_");
    fwrite(he_block.name, 1, 9, ftm_file);

    for (int i = 0; i < pr_block.channel; i++) {
        fwrite(&he_block.ch_id[i], 1, 1, ftm_file);
        fwrite(&he_block.ch_fx[i], 1, 1, ftm_file);
    }
}

void FTM_FILE::read_instrument_block() {
    fread(&inst_block, 1, sizeof(inst_block), ftm_file);
    DBG_PRINTF("\nINSTRUMENT HEADER: %s\n", inst_block.id);
    DBG_PRINTF("VERSION: %d\n", inst_block.version);
    DBG_PRINTF("SIZE: %d\n", inst_block.size);
    DBG_PRINTF("NUM_INST: 0x%X\n", inst_block.inst_num);
}

void FTM_FILE::write_instrument_block() {
    size_t tol_size = 4;
    for (int i = 0; i < inst_block.inst_num; i++) {
        tol_size += 311 + instrument[i].name_len;
    }
    printf("WRITE SIZE=%d\n", tol_size);
    inst_block.size = tol_size;
    fwrite(&inst_block, 1, sizeof(inst_block), ftm_file);
}

void FTM_FILE::read_instrument_data() {
    instrument.clear();
    DBG_PRINTF("\nREADING INSTRUMENT, COUNT=%d\n", inst_block.inst_num);
    for (int i = 0; i < inst_block.inst_num; i++) {
        instrument_t inst_tmp;
        DBG_PRINTF("\nREADING INSTRUMENT #%02X...\n", i);
        fread(&inst_tmp, sizeof(instrument_t) - 64, 1, ftm_file);
        fread(inst_tmp.name, inst_tmp.name_len, 1, ftm_file);
        inst_tmp.name[inst_tmp.name_len] = '\0';
        DBG_PRINTF("%02X->NAME: %s\n", inst_tmp.index, inst_tmp.name);
        DBG_PRINTF("TYPE: 0x%X\n", inst_tmp.type);
        DBG_PRINTF("SEQU COUNT: %d\n", inst_tmp.seq_count);
        DBG_PRINTF("SEQU DATA:\n");
        DBG_PRINTF("VOL: %d %d\n", inst_tmp.seq_index[0].enable, inst_tmp.seq_index[0].seq_index);
        DBG_PRINTF("ARP: %d %d\n", inst_tmp.seq_index[1].enable, inst_tmp.seq_index[1].seq_index);
        DBG_PRINTF("PIT: %d %d\n", inst_tmp.seq_index[2].enable, inst_tmp.seq_index[2].seq_index);
        DBG_PRINTF("HIP: %d %d\n", inst_tmp.seq_index[3].enable, inst_tmp.seq_index[3].seq_index);
        DBG_PRINTF("DTY: %d %d\n", inst_tmp.seq_index[4].enable, inst_tmp.seq_index[4].seq_index);
        DBG_PRINTF("\n");
        if (inst_tmp.index >= instrument.size()) {
            instrument.resize(inst_tmp.index + 1);
            instrument[inst_tmp.index] = inst_tmp;
        } else {
            instrument[inst_tmp.index] = inst_tmp;
        }
    }
    inst_block.inst_num = instrument.size();
}

void FTM_FILE::write_instrument_data() {
    for (int i = 0; i < inst_block.inst_num; i++) {
        printf("WRITE INSTRUMENT: %d\n", instrument[i].index);
        fwrite(&instrument[i], sizeof(instrument_t) - 64, 1, ftm_file);
        fwrite(instrument[i].name, instrument[i].name_len, 1, ftm_file);
    }
}

void FTM_FILE::read_sequences_block() {
    fread(&seq_block, 1, sizeof(seq_block), ftm_file);
    if (strncmp(seq_block.id, "SEQUENCES", 16)) {
        DBG_PRINTF("NO SEQUENCES!!\n");
        seq_block.version = 6;
        seq_block.sequ_num = 0;
        seq_block.size = 0;
        fseek(ftm_file, -sizeof(seq_block), SEEK_CUR);
        return;
    }
    DBG_PRINTF("\nSEQUENCES HEADER: %s\n", seq_block.id);
    DBG_PRINTF("VERSION: %d\n", seq_block.version);
    DBG_PRINTF("SIZE: %d\n", seq_block.size);
    DBG_PRINTF("NUM_SEQU: %d\n", seq_block.sequ_num);
}

void FTM_FILE::write_sequences() {
    size_t block_start_addr = ftell(ftm_file);
    int sequ_num = 0;
    fwrite(&seq_block, 1, sizeof(seq_block), ftm_file);

    size_t tol_size = 4;
    for (int type = 0; type < 5; type++) {
        for (int i = 0; i < sequences[type].size(); i++) {
            if (sequences[type][i].length) {
                sequences[type][i].type = type;
                sequences[type][i].index = i;
                fwrite(&sequences[type][i], 13, 1, ftm_file);
                fwrite(sequences[type][i].data.data(), 1, sequences[type][i].length, ftm_file);
                tol_size += 13 + sequences[type][i].length;
                sequ_num++;
            }
        }
    }
    for (int type = 0; type < 5; type++) {
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
    fwrite(&seq_block, 1, sizeof(seq_block), ftm_file);
    fseek(ftm_file, block_end_addr, SEEK_SET);
}

void FTM_FILE::read_sequences_data() {
    if (seq_block.sequ_num == 0) {
        return;
    }
    sequences.clear();
    sequences.resize(5);
    uint32_t index_map[seq_block.sequ_num][2];
    DBG_PRINTF("\nREADING SEQUENCES, COUNT=%d\n", seq_block.sequ_num);
    for (int i = 0; i < seq_block.sequ_num; i++) {
        sequences_t sequ_tmp;
        DBG_PRINTF("\nREADING SEQUENCES #%d...\n", i);
        fread(&sequ_tmp, 13, 1, ftm_file);
        sequ_tmp.data.resize(sequ_tmp.length);
        fread(sequ_tmp.data.data(), sequ_tmp.length, 1, ftm_file);
        DBG_PRINTF("#%d\n", sequ_tmp.index);
        DBG_PRINTF("TYPE: 0x%X\n", sequ_tmp.type);
        DBG_PRINTF("LENGTH: %d\n", sequ_tmp.length);
        DBG_PRINTF("LOOP: %d\n", sequ_tmp.loop);
        DBG_PRINTF("DATA:\n");
        for (int j = 0; j < sequ_tmp.length; j++) {
            DBG_PRINTF("%d ", sequ_tmp.data[j]);
        }
        if (sequ_tmp.index >= sequences[sequ_tmp.type].size()) {
            sequences[sequ_tmp.type].resize(sequ_tmp.index + 1);
            sequences[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
        } else {
            sequences[sequ_tmp.type][sequ_tmp.index] = sequ_tmp;
        }
        index_map[i][0] = sequ_tmp.type;
        index_map[i][1] = sequ_tmp.index;
        DBG_PRINTF("\n");
    }
    DBG_PRINTF("\nREADING SEQU SETTING...\n");
    for (int i = 0; i < seq_block.sequ_num; i++) {
        DBG_PRINTF("\n#%d: %d, %d\n", i, index_map[i][0], index_map[i][1]);
        fread(&sequences[index_map[i][0]][index_map[i][1]].release, sizeof(uint32_t), 1, ftm_file);
        DBG_PRINTF("RELEASE: %d\n", sequences[index_map[i][0]][index_map[i][1]].release);
        fread(&sequences[index_map[i][0]][index_map[i][1]].setting, sizeof(uint32_t), 1, ftm_file);
        DBG_PRINTF("SETTING: 0x%X\n", sequences[index_map[i][0]][index_map[i][1]].setting);
    }
    DBG_PRINTF("SECCESS.\n");
    if (1) { //getchar() == '1') {
        DBG_PRINTF("\nVOLUME:\n");
        for (uint32_t i = 0; i < sequences[0].size(); i++) {
            DBG_PRINTF("#%d: %d %d %d -> ", sequences[0][i].index, sequences[0][i].length, sequences[0][i].loop, sequences[0][i].release);
            for (int j = 0; j < sequences[0][i].length; j++) {
                DBG_PRINTF("%d ", sequences[0][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nARPEGGIO:\n");
        for (uint32_t i = 0; i < sequences[1].size(); i++) {
            DBG_PRINTF("#%d: %d %d %d -> ", sequences[1][i].index, sequences[1][i].length, sequences[1][i].loop, sequences[1][i].release);
            for (int j = 0; j < sequences[1][i].length; j++) {
                DBG_PRINTF("%d ", sequences[1][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nPITCH:\n");
        for (uint32_t i = 0; i < sequences[2].size(); i++) {
            DBG_PRINTF("#%d: %d %d %d -> ", sequences[2][i].index, sequences[2][i].length, sequences[2][i].loop, sequences[2][i].release);
            for (int j = 0; j < sequences[2][i].length; j++) {
                DBG_PRINTF("%d ", sequences[2][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nHI-PITCH:\n");
        for (uint32_t i = 0; i < sequences[3].size(); i++) {
            DBG_PRINTF("#%d: %d %d %d -> ", sequences[3][i].index, sequences[3][i].length, sequences[3][i].loop, sequences[3][i].release);
            for (int j = 0; j < sequences[3][i].length; j++) {
                DBG_PRINTF("%d ", sequences[3][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
        DBG_PRINTF("\nDUTY:\n");
        for (uint32_t i = 0; i < sequences[4].size(); i++) {
            DBG_PRINTF("#%d: %d %d %d -> ", sequences[4][i].index, sequences[4][i].length, sequences[4][i].loop, sequences[4][i].release);
            for (int j = 0; j < sequences[4][i].length; j++) {
                DBG_PRINTF("%d ", sequences[4][i].data[j]);
            }
            DBG_PRINTF("\n");
        }
    }
}

void FTM_FILE::read_frame_block() {
    fread(&fr_block, sizeof(FRAME_BLOCK), 1, ftm_file);
    DBG_PRINTF("\nFRAME HEADER: %s\n", fr_block.id);
    DBG_PRINTF("VERSION: %d\n", fr_block.version);
    DBG_PRINTF("SIZE: %d\n", fr_block.size);
    DBG_PRINTF("NUM_FRAMES: %d\n", fr_block.frame_num);
    DBG_PRINTF("PATTERN_LENGTH: %d\n", fr_block.pat_length);
    DBG_PRINTF("SPEED: %d\n", fr_block.speed);
    DBG_PRINTF("TEMPO: %d\n", fr_block.tempo);
}

void FTM_FILE::read_frame_data() {
    frames.clear();
    DBG_PRINTF("\nREADING FRAMES, COUNT=%d\n", fr_block.frame_num);
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
    printf("WRITE FRAME: %d\n", fr_block.frame_num);
    fwrite(&fr_block, sizeof(FRAME_BLOCK), 1, ftm_file);

    for (int f = 0; f < fr_block.frame_num; f++) {
        fwrite(frames[f].data(), 1, pr_block.channel, ftm_file);
        printf("FTELL: %d\n", ftell(ftm_file));
    }
}

void FTM_FILE::read_pattern_block() {
    fread(&pt_block, sizeof(PATTERN_BLOCK), 1, ftm_file);
    DBG_PRINTF("\nPATTERN HEADER: %s\n", pt_block.id);
    DBG_PRINTF("VERSION: %d\n", pt_block.version);
    DBG_PRINTF("SIZE: %d\n", pt_block.size);
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
        DBG_PRINTF("TRACK: %d\n", pt_tmp.track);
        DBG_PRINTF("CHANNEL: %d\n", pt_tmp.channel);
        DBG_PRINTF("INDEX: %d\n", pt_tmp.index);
        DBG_PRINTF("ITEMS: %d\n", pt_tmp.items);
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
                fwrite(&patterns[c][i].item[item], 1, item_size, ftm_file);
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
    fread(dpcm_block.id, 1, 16, ftm_file);
    DBG_PRINTF("DPCM HEADER: %s\n", dpcm_block.id);
    fread(&dpcm_block.version, 4, 1, ftm_file);
    DBG_PRINTF("VERSION: %d\n", dpcm_block.version);
    fread(&dpcm_block.size, 4, 1, ftm_file);
    DBG_PRINTF("SIZE: %d\n", dpcm_block.size);
    fread(&dpcm_block.sample_num, 2, 1, ftm_file);
    DBG_PRINTF("SAMPLE_NUM: %d\n", dpcm_block.sample_num);
}

void FTM_FILE::read_dpcm_data() {
    dpcm_samples.clear();
    if (dpcm_block.size == 1) {
        DBG_PRINTF("NO DPCM SAMPLE\n");
        return;
    }
    dpcm_samples.resize(dpcm_block.sample_num);
    DBG_PRINTF("RESIZE DPCM_SAMPLES TO %d\n", dpcm_samples.size());
    for (int i = 0; i < dpcm_block.sample_num; i++) {
        DBG_PRINTF("\n#%d\n", i);
        fread(&dpcm_samples[i].name_len, 4, 1, ftm_file);
        fread(dpcm_samples[i].name, 1, dpcm_samples[i].name_len, ftm_file);
        DBG_PRINTF("NAME(SIZE=%d): %s\n", dpcm_samples[i].name_len, dpcm_samples[i].name);
        fread(&dpcm_samples[i].sample_size_byte, 4, 1, ftm_file);
        dpcm_samples[i].dpcm_data.resize(dpcm_samples[i].sample_size_byte + 1);
        // DBG_PRINTF("FTELL: %d\n", ftell(ftm_file));
        fread(dpcm_samples[i].dpcm_data.data(), 1, dpcm_samples[i].sample_size_byte + 1, ftm_file);
        DBG_PRINTF("SAMPLE SIZE(byte) = %d\n", dpcm_samples[i].sample_size_byte);
        DBG_PRINTF("DECODE...\n");
        dpcm_samples[i].pcm_data.resize((dpcm_samples[i].sample_size_byte + 1) * 8);
        decode_dpcm(dpcm_samples[i].dpcm_data.data(), dpcm_samples[i].sample_size_byte + 1, dpcm_samples[i].pcm_data.data());
        // DBG_PRINTF("FTELL: %d\n", ftell(ftm_file));
        DBG_PRINTF("SECCESS.\n");
    }
}

void FTM_FILE::write_dpcm() {
    size_t block_start_addr = ftell(ftm_file);
    fwrite(&dpcm_block, sizeof(DPCM_SAMPLE_BLOCK), 1, ftm_file);

    const uint8_t zero = 0;
    
    if (dpcm_block.sample_num == 0) {
        return;
    }
    
    size_t tol_size = 2;
    for (int i = 0; i < dpcm_samples.size(); i++) {
        fwrite(&dpcm_samples[i].name_len, 4, 1, ftm_file);
        fwrite(dpcm_samples[i].name, 1, dpcm_samples[i].name_len, ftm_file);
        fwrite(&dpcm_samples[i].sample_size_byte, 4, 1, ftm_file);
        fwrite(dpcm_samples[i].dpcm_data.data(), 1, dpcm_samples[i].sample_size_byte + 1, ftm_file);
        tol_size += 4 + dpcm_samples[i].name_len + 4 + dpcm_samples[i].sample_size_byte + 1;
    }

    size_t block_end_addr = ftell(ftm_file);

    fseek(ftm_file, block_start_addr, SEEK_SET);
    fwrite(&dpcm_block, sizeof(DPCM_SAMPLE_BLOCK), 1, ftm_file);
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

void FTM_FILE::resize_sequ(int type, int size) {
    if (type >= sequences.size()) return;
    sequences[type].resize(size);
    sequences[type][size - 1].type = type;
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

FTM_FILE ftm;