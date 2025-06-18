#include "gui_tracker.h"
#include "gui_menu.h"      // for menu()
#include "gui_input.h"     // for change_edit_mode()
#include "gui_channel.h"   // for channel_setting_page()
#include "gui_file.h"      // for menu_file()
#include "gui_settings.h"  // for settings_page()

// Copy pattern data from startRow (inclusive) to endRow (exclusive) for a given channel into clipboard_data
void copy_data(int startRow, int endRow, int channel) {
    if (endRow <= startRow) return;
    clipboard_data.clear();
    clipboard_data.reserve(endRow - startRow);
    for (int r = startRow; r < endRow; ++r) {
        unpk_item_t item = ftm.get_pt_item(channel, player.get_cur_frame_map(channel), r);
        clipboard_data.push_back(item);
    }
}

void paste_data(int startRow, int channel) {
    if (!edit_mode) return;
    for (size_t i = 0; i < clipboard_data.size(); ++i) {
        int destRow = startRow + i;
        // Ensure destination exists
        if (destRow < ftm.unpack_pt[channel][player.get_cur_frame_map(channel)].size()) {
            ftm.set_pt_item(channel, player.get_cur_frame_map(channel), destRow, clipboard_data[i]);
        }
    }
}

void clear_clipboard() {
    clipboard_data.clear();
}

void clipboard_page() {
    const char *menu_str[2] = {"COPY", "PASTE"};
    int ret = menu("CLIPBOARD", menu_str, 2, NULL, 64, 29, 0, 0, 0);
    switch (ret)
    {
    case 0:
        copy_mode = true;
        copy_start = player.get_row();
        break;

    case 1:
        copy_mode = false;
        drawPopupBox("PROCESS...");
        display.display();
        paste_data(player.get_row(), channel_sel_pos);
        break;
    
    default:
        break;
    }
}

void fast_inst_sel_menu() {
    drawPopupBox("LOADING...");
    display.display();
    char *inst_name[ftm.inst_block.inst_num];
    for (int i = 0; i < ftm.inst_block.inst_num; i++) {
        inst_name[i] = new char[32];
        snprintf(inst_name[i], 32, "%02X-%.20s", i, ftm.get_inst(i)->name);
    }

    menu("INSTRUMEN INDEX", (const char**)inst_name, ftm.inst_block.inst_num, NULL, 100, 60, 0, 0, inst_sel_pos, &inst_sel_pos);

    drawPopupBox("CLEAR...");
    display.display();
    for (int i = 0; i < ftm.inst_block.inst_num; i++) {
        delete[] inst_name[i];
    }
}

void tracker_menu() {
    display.setFont(&rismol35);
    display.setTextWrap(false);
    display.setTextColor(1);
    display.setTextSize(1);
    for (;;) {
        display.clearDisplay();
        display.setCursor(2, 0);
        display.print("R");
        display.setCursor(14, 0);
        display.print("PU1   PU2   TRI   NOS   DMC");
        display.drawFastHLine(0, 6, 128, 1);
        display.setCursor(0, 11);
        display.drawFastHLine(0, 10, 128, 1);
        drawChessboard(0, 7, 7, 3);
        // drawChessboard(8, 7, 23, 3);
        for (int c = 0; c < 5; c++) {
            if (player.get_mute(c))
                drawChessboard((c * 24) + 8, 7, 23, 3);

            display.fillRect((c * 24) + 8, 7, roundf(player.channel[c].get_rel_vol() * (23.0f/225.0f)), 3, 1);
        }
        for (int r = -4; r < 5; r++) {
            if (r) {
                display.setTextColor(1);
            } else {
                display.fillRect(0, display.getCursorY() - 1, 128, 7, 1);
                display.setTextColor(0);
            }
            if (((player.get_row() + r) >= 0) && ((player.get_row() + r) < ftm.fr_block.pat_length)) {
                display.printf("%02X ", player.get_row() + r);
                display.setCursor(display.getCursorX() - 2, display.getCursorY());
                if (copy_mode) {
                    if (((player.get_row() + r) >= copy_start) && (r <= 0)) {
                        drawChessboard((channel_sel_pos * 24) + 8, display.getCursorY(), 23, 6);
                    }
                }
                for (int i = 0; i < 5; i++) {
                    unpk_item_t pt_tmp = ftm.get_pt_item(i, player.get_cur_frame_map(i), player.get_row() + r);
                    if (pt_tmp.note != NO_NOTE) {
                        if (pt_tmp.note == NOTE_CUT)
                            display.printf("^^^ ");
                        else if (pt_tmp.note == NOTE_END)
                            display.printf("~~~ ");
                        else {
                            if (i == 3)
                                display.printf("%X-# ", note2noise(item2note(pt_tmp.note, pt_tmp.octave)));
                            else
                                display.printf("%s%d ", note2str[pt_tmp.note], pt_tmp.octave);
                        }
                    } else {
                        display.printf("... ");
                    }
                    if (pt_tmp.volume != NO_VOL) {
                        display.printf("%X ", pt_tmp.volume);
                    } else {
                        display.printf(". ");
                    }
                }
            }
            display.printf("\n");
        }
        display.drawFastVLine(7, 0, 64, 1);
        for (int i = 0; i < 5; i++) {
            display.drawFastVLine((i * 24) + 31, 0, 64, 1);
            if (i == channel_sel_pos) {
                display.setCursor((i * 24) + 9, 0);
                display.print(">");
                if (edit_mode) {
                    invertRect((i * 24) + 8, 0, 23, 6);
                }
            }
        }
        display.display();

        if (touchKeypad.available()) {
            uint8_t note_set = 0;
            uint8_t octv_set = 0;
            uint8_t vol_set = NO_VOL;

            if (touchKeypad.available()) {
                touchKeypadEvent e = touchKeypad.read();
                update_touchpad_note(&note_set, &octv_set, e);
            }

            if (note_set) {
                DBG_PRINTF("INPUT_NOTE_SET: %d\n", note_set);
                if (edit_mode) {
                    unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                    pt_tmp.note = note_set;
                    if (note_set > 12) {
                        pt_tmp.octave = NO_OCT;
                        pt_tmp.instrument = NO_INST;
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
                } else if (e.bit.KEY == KEY_BACK) {
                    if (edit_mode) {
                        unpk_item_t pt_tmp;
                        ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                    }
                    copy_mode = false;
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {
                    main_option_page();
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
                    set_channel_sel_pos(channel_sel_pos - 1);
                } else if (e.bit.KEY == KEY_R) {
                    set_channel_sel_pos(channel_sel_pos + 1);
                } else if (e.bit.KEY == KEY_OCTU) {
                    g_octv++;
                } else if (e.bit.KEY == KEY_OCTD) {
                    g_octv--;
                }
            }
        }
        vTaskDelay(2);
    }
}

void main_option_page() {
    drawChessboard(0, 0, 128, 64);
    const char *menu_str[10] = {edit_mode ? "STOP EDIT" : "START EDIT", 
                                player.get_mute(channel_sel_pos) ? "UNMUTE" : "MUTE",
                                "INSTRUMENT", "CLIPBOARD", "CHANNEL", "FILE", "SETTINGS", "VOLUME", "TEST KEYBOARD", "REBOOT"};
    int ret = menu("OPTION", menu_str, 10, NULL, 64, 45, 0, 0, 0);
    switch (ret)
    {
    case 0:
        edit_mode = !edit_mode;
        break;
    case 1:
        player.set_mute(channel_sel_pos, !player.get_mute(channel_sel_pos));
        break;
    case 2:
        fast_inst_sel_menu();
        break;
    case 3:
        clipboard_page();
        break;
    case 4:
        channel_setting_page();
        break;
    case 5:
        menu_file();
        break;
    case 6:
        settings_page();
        break;
    case 7:
        vol_set_page();
        break;
    case 8:
        test_displayKeyboard();
        break;
    case 9:
        reboot_page();
        break;
    default:
        break;
    }
}