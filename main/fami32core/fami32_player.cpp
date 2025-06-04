#include "fami32_player.h"

void FAMI_PLAYER::init(FTM_FILE* data) {
    ftm_data = data;
    for (int c = 0; c < 5; c++) {
        channel[c].init(ftm_data);
    }

    channel[2].set_mode(TRIANGULAR);
    channel[3].set_mode(NOISE0);

    channel[4].set_mode(DPCM_SAMPLE);

    buf.resize(channel[0].get_buf_size());

    set_speed(ftm_data->fr_block.speed);
    set_tempo(ftm_data->fr_block.tempo);

    hpf.setCutoffFrequency(HPF_CUTOFF, SAMP_RATE);
    lpf.setCutoffFrequency(LPF_CUTOFF, SAMP_RATE);
}

void FAMI_PLAYER::reload() {
    stop_play();

    set_speed(ftm_data->fr_block.speed);
    set_tempo(ftm_data->fr_block.tempo);
    for (int c = 0; c < 5; c++) {
        channel[c].init(ftm_data);
    }

    frame = 0;
    row = 0;
}

void FAMI_PLAYER::start_play() {
    set_speed(ftm_data->fr_block.speed);
    set_tempo(ftm_data->fr_block.tempo);
    play_status = true;
    tick_accumulator = ticks_row;
    row = 0;
    // frame = 0;
}

void FAMI_PLAYER::stop_play() {
    play_status = false;
    for (int c = 0; c < 5; c++) {
        channel[c].note_cut();
        channel[c].clear_all_fx_flag();
        memset(channel[c].get_buf(), 0, channel[c].get_buf_size_byte());
    }
    tick_accumulator = 0;
    // memset(buf.data(), 0, get_buf_size_byte());
}

void FAMI_PLAYER::ref_speed_and_tempo() {
    set_speed(ftm_data->fr_block.speed);
    set_tempo(ftm_data->fr_block.tempo);
}

void FAMI_PLAYER::set_speed(int speed_ref) {
    if (speed_ref < 1) speed_ref = 1;
    speed = speed_ref;
    recalculate_ticks_row();
}

void FAMI_PLAYER::set_tempo(int tempo_ref) {
    tempo = tempo_ref;
    recalculate_ticks_row();
}

int FAMI_PLAYER::get_speed() const {
    return speed;
}

int FAMI_PLAYER::get_tempo() const {
    return tempo;
}

void FAMI_PLAYER::set_ticks_row(float tr) {
    ticks_row = tr;
}

float FAMI_PLAYER::get_ticks_row() const {
    return ticks_row;
}

void FAMI_PLAYER::mix_all_channel() {
    for (int c = 0; c < 5; c++) {
        channel[c].update_tick();
    }
    for (size_t i = 0; i < buf.size(); i++) {
        int32_t r = 0;
        for (int c = 0; c < 5; c++) {
            if (!mute[c])
                r += channel[c].get_buf()[i];
        }

        r = hpf.process(r);
        r = lpf.process(r);
        buf[i] = (r * g_vol) >> 5;
    }
}

uint8_t FAMI_PLAYER::get_chl_vol(int n) {
    return channel[n].get_vol();
}

int8_t FAMI_PLAYER::get_chl_env_vol(int n) {
    return channel[n].get_env_vol();
}

void FAMI_PLAYER::process_delay_efx(fx_t fxdata[4], int c) {
    uint8_t count = ftm_data->ch_fx_count(c);
    for (uint8_t i = 0; i < count; i++) {
        if (fxdata[i].fx_cmd == 0x0E) {
            if (fxdata[i].fx_var >= ticks_row) {
                DBG_PRINTF("C%d: WARN DELAY OUTOF TICKSROW\n", c);
                delay_count[c] = (uint8_t)ticks_row;
                delay_status[c] = true;
            } else {
                delay_count[c] = fxdata[i].fx_var;
                delay_status[c] = true;
                DBG_PRINTF("C%d: SET DELAY -> %d\n", c, delay_count[c]);
            }
        }
    }
}

void FAMI_PLAYER::process_efx_pre(fx_t fxdata[4], int c) {
    uint8_t count = ftm_data->ch_fx_count(c);
    for (uint8_t i = 0; i < count; i++) {
        if (fxdata[i].fx_cmd == 0x01) {
            if (fxdata[i].fx_var >= 0x20) {
                set_tempo(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET TEMPO -> %d\n", c, fxdata[i].fx_var);
            } else {
                set_speed(fxdata[i].fx_var);
                DBG_PRINTF("C%d: SET SPEED -> %d\n", c, fxdata[i].fx_var);
            }
        } else if (fxdata[i].fx_cmd == 0x02) {
            jmp_to_frame(fxdata[i].fx_var);
            DBG_PRINTF("C%d: JMP TO FRAME -> %02X\n", c, fxdata[i].fx_var);
        } else if (fxdata[i].fx_cmd == 0x03) {
            next_frame(fxdata[i].fx_var - 1);
            DBG_PRINTF("C%d: SKIP TO NEXT FRAME ROW %d\n", c, fxdata[i].fx_var);
        } else if (fxdata[i].fx_cmd == 0x04) {
            stop_play();
            DBG_PRINTF("SONG HALT\n");
        } else if (fxdata[i].fx_cmd == 0x06) {
            channel[c].set_auto_port(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET AUTO_PORT -> %d\n", c, fxdata[i].fx_var);
        } else if (fxdata[i].fx_cmd == 0x0A) {
            channel[c].set_arp_fx(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET ARP -> N1 +%d, N2 +%d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
        } else if (fxdata[i].fx_cmd == 0x0B) {
            channel[c].set_vibrato(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET VIBRATO SPEED -> %d, DEEP -> %d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
        } else if (fxdata[i].fx_cmd == 0x0C) {
            channel[c].set_tremolo(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET TREMOLO SPEED -> %d, DEEP -> %d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
        } else if (fxdata[i].fx_cmd == 0x0D) {
            channel[c].set_period_offset(fxdata[i].fx_var - 0x80);
            DBG_PRINTF("C%d: SET PERIOD_OFFSET -> %d\n", c, fxdata[i].fx_var - 0x80);
        } else if (fxdata[i].fx_cmd == 0x0F) {
            channel[c].set_dpcm_var(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET DPCM VAR -> %d\n", c, fxdata[i].fx_var);
        } else if ((fxdata[i].fx_cmd == 0x10)) {// || (fxdata[i].fx_cmd == 0x08)) {
            channel[c].set_slide_up(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET SLIDE_UP -> %d\n", c, fxdata[i].fx_var);
        } else if ((fxdata[i].fx_cmd == 0x11)) {//|| (fxdata[i].fx_cmd == 0x09)) {
            channel[c].set_slide_down(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET SLIDE_DOWN -> %d\n", c, fxdata[i].fx_var);
        } else if (fxdata[i].fx_cmd == 0x12) {
            channel[c].set_chl_mode((WAVE_TYPE)(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET MODE -> %d\n", c, fxdata[i].fx_var);
        } else if (fxdata[i].fx_cmd == 0x13) {
            channel[c].set_dpcm_offset(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET DPCM OFFSET %d Bytes (%d Samples)\n", c, fxdata[i].fx_var, fxdata[i].fx_var * 8);
        }else if (fxdata[i].fx_cmd == 0x16) {
            channel[c].set_vol_slide_up(HEX_B1(fxdata[i].fx_var));
            channel[c].set_vol_slide_down(HEX_B2(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET VOL_SLIDE, UP->%d - DOWN->%d = %d\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var), HEX_B1(fxdata[i].fx_var) - HEX_B2(fxdata[i].fx_var));
        } else if (fxdata[i].fx_cmd == 0x17) {
            channel[c].set_delay_cut(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET DELAY_CUT -> %d\n", i, fxdata[i].fx_var);
        } else if (fxdata[i].fx_cmd == 0x1D) {
            channel[c].set_dpcm_pitch(fxdata[i].fx_var);
            DBG_PRINTF("C%d: SET DPCM PITCH -> %d\n", i, fxdata[i].fx_var);
        }
    }
}

void FAMI_PLAYER::process_efx_post(fx_t fxdata[4], int c) {
    uint8_t count = ftm_data->ch_fx_count(c);
    for (uint8_t i = 0; i < count; i++) {
        if (fxdata[i].fx_cmd == 0x14) {
            channel[c].set_port_up(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET PORT_UP SPEED -> %d, +%dNOTE\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
        } else if (fxdata[i].fx_cmd == 0x15) {
            channel[c].set_port_down(HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
            DBG_PRINTF("C%d: SET PORT_DOWN SPEED -> %d, +%dNOTE\n", c, HEX_B1(fxdata[i].fx_var), HEX_B2(fxdata[i].fx_var));
        }
    }
}

void FAMI_PLAYER::process_item(unpk_item_t item, int c) {
    process_efx_pre(item.fxdata, c);
    if (item.instrument != NO_INST) {
        channel[c].set_inst(item.instrument);
    }
    if (item.note != NO_NOTE) {
        if (item.note == NOTE_END) {
            channel[c].note_end();
#ifndef DESKTOP_BUILD
            if (_midi_output && !mute[c]) {
                MIDI.noteOff(channel[c].base_note, 0, c);
            }
#endif
        } else if (item.note == NOTE_CUT) {
            channel[c].note_cut();
#ifndef DESKTOP_BUILD
            if (_midi_output && !mute[c]) {
                MIDI.noteOff(channel[c].base_note, 0, c);
            }
#endif
        } else {
#ifndef DESKTOP_BUILD
            if (_midi_output && !mute[c]) {
                MIDI.noteOff(channel[c].base_note, 0, c);
            }
#endif
            channel[c].set_note(item2note(item.note, item.octave));
            channel[c].note_start();
#ifndef DESKTOP_BUILD
            if (_midi_output && !mute[c]) {
                MIDI.noteOn(channel[c].base_note, (item.volume == NO_VOL ? channel[c].get_vol() : item.volume) << 3, c);
            }
#endif
        }
    }
    if (item.volume != NO_VOL) {
        channel[c].set_vol(item.volume);
#ifndef DESKTOP_BUILD
        if (_midi_output && !mute[c]) {
            MIDI.controlChange(7, item.volume << 3, c);
        }
#endif
        // DBG_PRINTF("SET_VOL(C%d): %d\n", c, channel[c].get_vol());
    }
    process_efx_post(item.fxdata, c);
}

void FAMI_PLAYER::process_tick() {
    // printf("PROCESS TICK, STATUS -> %d\n", play_status);
    if (!play_status) {
        mix_all_channel();
        return;
    }
    for (int c = 0; c < 5; c++) {
        if (!delay_status[c]) {
            continue;
        }
        if (delay_count[c]) {
            delay_count[c]--;
            if (!delay_count[c]) {
                process_item(ft_items[c], c);
                delay_status[c] = false;
            }
        }
    }

    while (tick_accumulator >= ticks_row) {
        for (int c = 0; c < 5; c++) {
            ft_items[c] = ftm_data->get_pt_item(c, ftm_data->get_frame_map(frame, c), row);
            // process_efx(ft_items[c].fxdata, c);
            process_delay_efx(ft_items[c].fxdata, c);
            if (!delay_count[c])
                process_item(ft_items[c], c);
        }

        row++;
        if (row >= ftm_data->fr_block.pat_length) {
            next_frame(0);
        }

        tick_accumulator -= ticks_row;
    }

    tick_accumulator += 1.0f;
    mix_all_channel();
}

void FAMI_PLAYER::next_frame(int r) {
    row = r;
    frame++;
    while (frame >= ftm_data->fr_block.frame_num) {
        frame -= ftm_data->fr_block.frame_num;
    }
}

void FAMI_PLAYER::jmp_to_frame(int f) {
    if (f >= ftm_data->fr_block.frame_num) {
        f = 0;
    } else if (f < 0) {
        f = ftm_data->fr_block.frame_num - 1;
    }
    frame = f;
    row = -1;
}

int16_t* FAMI_PLAYER::get_buf() {
    return buf.data();
}

size_t FAMI_PLAYER::get_buf_size_byte() const {
    return buf.size() * sizeof(int16_t);
}

size_t FAMI_PLAYER::get_buf_size() const {
    return buf.size();
}

int FAMI_PLAYER::get_row() {
    return row;
}

void FAMI_PLAYER::set_row(int r) {
    if (r >= ftm_data->fr_block.pat_length) r = 0;
    else if (r < 0) r = ftm_data->fr_block.pat_length;
    row = r;
}

int FAMI_PLAYER::get_frame() {
    return frame;
}

uint8_t FAMI_PLAYER::get_cur_frame_map(int c) {
    return ftm_data->get_frame_map(frame, c);
}

bool FAMI_PLAYER::get_play_status() {
    return play_status;
}

void FAMI_PLAYER::set_mute(int c, bool s) {
    mute[c] = s;
}

bool FAMI_PLAYER::get_mute(int c) {
    return mute[c];
}

void FAMI_PLAYER::recalculate_ticks_row() {
    ticks_row = (float)(150 * speed) / (float)tempo;
    tick_accumulator = ticks_row;
}