#include "gui_channel.h"
#include "gui_menu.h"       // for menu(), drawPinstripe
#include "gui_input.h"      // for set_channel_sel_pos()
#include "gui_instrument.h" // for instrument_menu (if needed)
#include "gui_tracker.h"    // for menu_navi(), main_option_page()

// Simple menu to select current channel (0-4)
void channel_sel_page() {
    static const char *chan_str[5] = {"PULSE1", "PULSE2", "TRIANGLE", "NOISE", "DPCM"};
    int ret = menu("CHANNEL", chan_str, 5, nullptr, 64, 53, 0, 0, channel_sel_pos);
    if (ret != -1) {
        set_channel_sel_pos(ret);
    }
}

// Channel settings menu: select channel or adjust extended effect count
void channel_setting_page() {
    drawPinstripe(0, 0, 128, 64);
    static const char *options[2] = {"SELECT CHAN", "EXT EFX NUM"};
    char title[16];
    snprintf(title, sizeof(title), "CHANNEL%d", channel_sel_pos);
    int ret = menu(title, options, 2, nullptr, 64, 29);
    int tmp = ftm.he_block.ch_fx[channel_sel_pos];
    switch (ret) {
        case 0:  // Select channel (brings up channel selection menu)
            channel_sel_page();
            break;
        case 1:  // Set number of effect columns for this channel
            drawPinstripe(0, 0, 128, 64);
            num_set_menu_int("EXT EFX NUM", 0, 3, 1, &tmp, 0, 0, 64, 32);
            ftm.he_block.ch_fx[channel_sel_pos] = tmp;
            break;
        default:
            break;
    }
}

uint8_t fx_help_menu() {
    static const char *fx_help_str[20] = {
        "Fxx: Set Tempo(x<10)/Speed(x>10)",
        "Bxx: Jump to frame xx",
        "Dxx: Jump to next frame's row xx",
        "Cxx: Halt song",
        "3xx: Auto Portamento,xx = speed",
        "0xy: Arp, x=2nd note,y=3rd note",
        "4xy: Vibrato, x=speed, y=depth",
        "7xy: Tremolo, x=speed, y=depth",
        "Pxx: Fine pitch (ofst = xx - 80)",
        "Gxx: Row delay, x = Num Of Tick",
        "Zxx: Set DMC Channel Level",
        "1xx: Slide up, xx = speed",
        "2xx: Slide down, xx = speed",
        "V0x: Set Duty/Mode, x = mode",
        "Yxx: Set DMC Offset, xx = offset",
        "Qxy: PortUp, x=speed, y=+note",
        "Rxy: PortDown, x=speed, y=-note",
        "Axy: VolSlide, x=up, y=down",
        "Sxx: Delay cut, x = Num Of Tick",
        "W0x: Set DMC pitch"
    };
    uint8_t ret = fullScreenMenu("EFFECT HELP", fx_help_str, 20, NULL, 0);
    if (ret == 255) return NO_EFX;
    return vaild_fx_table[ret];
}

void channel_menu() {
    static uint8_t x_pos = 0;
    display.setFont(&font3x5);
    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.printf("%s ", ch_name[channel_sel_pos]);
        display.setFont(&font4x6);
        display.printf("(CHAN%d)", channel_sel_pos);

        display.setFont(&rismol35);
        display.setTextColor(1);

        display.setCursor(0, 11);
        display.drawFastHLine(0, 10, 128, 1);
        display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);

        for (int r = -4; r < 5; r++) {
            if (r) {
                display.setTextColor(1);
            } else {
                display.fillRect(0, 34, 59 + (ftm.he_block.ch_fx[channel_sel_pos] * 16), 7, 1);
                display.setTextColor(0);
            }
            if (((player.get_row() + r) >= 0) && ((player.get_row() + r) < ftm.fr_block.pat_length)) {
                display.printf("%02X ", player.get_row() + r);
                display.setCursor(display.getCursorX() - 2, display.getCursorY());

                if (copy_mode) {
                    if (((player.get_row() + r) >= copy_start) && (r <= 0)) {
                        drawChessboard(9, display.getCursorY(), 50 + (ftm.he_block.ch_fx[channel_sel_pos] * 16), 6);
                    }
                }

                unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row() + r);
                if (pt_tmp.note != NO_NOTE) {
                    if (pt_tmp.note == NOTE_CUT)
                        display.printf("--- ");
                    else if (pt_tmp.note == NOTE_END)
                        display.printf("=== ");
                    else {
                        if (channel_sel_pos == 3)
                            display.printf("%X-# ", note2noise(item2note(pt_tmp.note, pt_tmp.octave)));
                        else
                            display.printf("%s%d ", note2str[pt_tmp.note], pt_tmp.octave);
                    }
                } else {
                    display.printf("... ");
                }
                if (pt_tmp.instrument != NO_INST) {
                    display.printf("%02X ", pt_tmp.instrument);
                } else {
                    display.printf(".. ");
                }
                if (pt_tmp.volume != NO_VOL) {
                    display.printf("%X", pt_tmp.volume);
                } else {
                    display.printf(".");
                }
                for (uint8_t fx = 0; fx < ftm.he_block.ch_fx[channel_sel_pos] + 1; fx++) {
                    if (pt_tmp.fxdata[fx].fx_cmd) {
                        display.printf(" %c%02X", fx2char[pt_tmp.fxdata[fx].fx_cmd], pt_tmp.fxdata[fx].fx_var);
                    } else {
                        display.printf(" ...");
                    }
                }
            }
            display.printf("\n");
        }
        display.drawFastVLine(8, 11, 53, 1);
        // display.drawFastVLine(59 + (ftm.he_block.ch_fx[channel_sel_pos] * 16), 11, 53, 1);
        if (ftm.he_block.ch_fx[channel_sel_pos] < 3)
            display.fillRect(59 + (ftm.he_block.ch_fx[channel_sel_pos] * 16), 11, 106 - (59 + (ftm.he_block.ch_fx[channel_sel_pos] * 16)), 53, 1);
        else
            display.drawFastVLine(106, 11, 53, 1);

        if (edit_mode) {
            if (x_pos < 4) {
                if (x_pos == 0) {
                    invertRect(9, 33, 13, 9);
                } else if (x_pos == 1) {
                    invertRect(25, 33, 5, 9);
                } else if (x_pos == 2) {
                    invertRect(29, 33, 5, 9);
                } else if (x_pos == 3) {
                    invertRect(37, 33, 5, 9);
                }
            } else {
                int x = ((x_pos - 1) * 4) + (((x_pos - 1) / 3) * 4) + 29;
                invertRect(x, 33, 5, 9);
            }
        }

        display.setCursor(107, 12);
        if (player.get_mute(channel_sel_pos)) {
            display.printf("MUTE\n\n");
        } else {
            display.printf("\n\n");
        }
        display.setCursor(107, display.getCursorY());
        display.printf("CHAN:\n");
        display.setCursor(107, display.getCursorY());
        display.printf("%ld\n\n", player.channel[channel_sel_pos].get_inst()->index);
        display.setCursor(107, display.getCursorY());
        display.printf("SEL:\n");
        display.setCursor(107, display.getCursorY());
        display.printf("%d\n\n", inst_sel_pos);
        display.setCursor(107, display.getCursorY() - 1);
        display.printf("FX:%d", ftm.ch_fx_count(channel_sel_pos));

        display.display();

        uint8_t note_set = 0;
        uint8_t octv_set = 0;
        uint8_t vol_set = NO_VOL;

        if (edit_mode) {
            if (x_pos == 0) {
                if (touchKeypad.available()) {
                    touchKeypadEvent e = touchKeypad.read();
                    update_touchpad_note(&note_set, &octv_set, e);
                }
                if (note_set) {
                    printf("INPUT_NOTE_SET: %d\n", note_set);
                    unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                    pt_tmp.note = note_set;
                    if (pt_tmp.note > 12) {
                        pt_tmp.octave = 0;
                    } else {
                        pt_tmp.octave = octv_set;
                        pt_tmp.instrument = inst_sel_pos;
                        if (vol_set != NO_VOL) {
                            pt_tmp.volume = vol_set;
                        }
                    }
                    ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                    if (!player.get_play_status()) {
                        player.set_row(player.get_row() + 1);
                    }
                }
            } else if (touchKeypad.available()) {
                touchKeypadEvent e = touchKeypad.read();
                if (e.bit.EVENT == KEY_JUST_PRESSED) {
                    unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                    if (x_pos == 1) {
                        pt_tmp.instrument = SET_HEX_B1(pt_tmp.instrument, e.bit.KEY);
                    } else if (x_pos == 2) {
                        pt_tmp.instrument = SET_HEX_B2(pt_tmp.instrument, e.bit.KEY);
                    } else if (x_pos == 3) {
                        pt_tmp.volume = e.bit.KEY;
                    } else if (x_pos >= 4) {
                        if (((x_pos - 4) % 3) == 1)
                            pt_tmp.fxdata[(x_pos - 4) / 3].fx_var = SET_HEX_B1(pt_tmp.fxdata[(x_pos - 4) / 3].fx_var, e.bit.KEY);
                        else if (((x_pos - 4) % 3) == 2)
                            pt_tmp.fxdata[(x_pos - 4) / 3].fx_var = SET_HEX_B2(pt_tmp.fxdata[(x_pos - 4) / 3].fx_var, e.bit.KEY);
                        else
                            pt_tmp.fxdata[(x_pos - 4) / 3].fx_cmd = fast_fx_table[e.bit.KEY];
                    }
                    ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                }
            }
        } else {
            if (touchKeypad.available()) {
                touchKeypadEvent e = touchKeypad.read();
                update_touchpad_note(&note_set, &octv_set, e);
            }
        }

        // KEYPAD
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (copy_mode) {
                        copy_mode = false;
                        drawPopupBox("PROCESS...");
                        display.display();
                        copy_data(copy_start, player.get_row() + 1, channel_sel_pos);
                    } else {
                        if (player.get_play_status()) {
                            pause_sound();
                        } else {
                            start_sound();
                        }
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_BACK) {
                    if (edit_mode) {
                        unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                        if (x_pos == 0) {
                            pt_tmp.note = NO_NOTE;
                            pt_tmp.instrument = NO_INST;
                        } else if (x_pos == 1 || x_pos == 2) {
                            pt_tmp.instrument = NO_INST;
                        } else if (x_pos == 3) {
                            pt_tmp.volume = e.bit.KEY;
                        } else if (x_pos >= 4) {
                            pt_tmp.fxdata[(x_pos - 4) / 3].fx_cmd = NO_EFX;
                            pt_tmp.fxdata[(x_pos - 4) / 3].fx_var = 0;
                        }
                        ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                    } else {
                        drawPinstripe(0, 0, 128, 64);
                        channel_sel_page();
                    }
                    copy_mode = false;
                } else if (e.bit.KEY == KEY_MENU) {
                    if (edit_mode && (((x_pos - 4) % 3) == 0)) {
                        unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                        pt_tmp.fxdata[(x_pos - 4) / 3].fx_cmd = fx_help_menu();
                        ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                    } else {
                        main_option_page();
                    }
                } else if (e.bit.KEY == KEY_P) {
                    player.jmp_to_frame(player.get_frame() + 1);
                    player.set_row(player.get_row() + 1);
                } else if (e.bit.KEY == KEY_S) {
                    player.jmp_to_frame(player.get_frame() - 1);
                    player.set_row(player.get_row() + 1);
                } else if (e.bit.KEY == KEY_UP) {
                    player.set_row(player.get_row() - 1);
                } else if (e.bit.KEY == KEY_DOWN) {
                    player.set_row(player.get_row() + 1);
                } else if (e.bit.KEY == KEY_L) {
                    if (edit_mode) {
                        x_pos--;
                    } else {
                        set_channel_sel_pos(channel_sel_pos - 1);
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (edit_mode) {
                        x_pos++;
                    } else {
                        set_channel_sel_pos(channel_sel_pos + 1);
                    }
                } else if (e.bit.KEY == KEY_OCTU) {
                    g_octv++;
                } else if (e.bit.KEY == KEY_OCTD) {
                    g_octv--;
                }
                x_pos %= 4 + (ftm.ch_fx_count(channel_sel_pos) * 3);
            }
        }

        vTaskDelay(2);
    }
}
