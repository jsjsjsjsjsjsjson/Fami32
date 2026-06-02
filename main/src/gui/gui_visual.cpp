#include "gui_visual.h"
#include "gui_menu.h"      // for drawChessboard, drawPinstripe
#include "gui_tracker.h"   // for menu_navi(), main_option_page()
#include "gui_input.h"

static uint8_t visual_visible_first_channel() {
    uint8_t count = player.get_channel_count();
    uint8_t visible = count < 5 ? count : 5;
    if (count <= visible) return 0;
    int first = channel_sel_pos - 2;
    if (first < 0) first = 0;
    int max_first = count - visible;
    if (first > max_first) first = max_first;
    return first;
}

static uint8_t visual_visible_channel_count() {
    uint8_t count = player.get_channel_count();
    return count < 5 ? count : 5;
}

static const char *visual_channel_label(uint8_t channel, char *buf, size_t len) {
    if (len < 5) {
        return "";
    }
    if (channel < 5) {
        static const char *base_short[5] = {"PU1", "PU2", "TRI", "NOS", "DMC"};
        return base_short[channel];
    }
    if (ftm.is_fds_channel(channel)) {
        return "FDS";
    }
    if (ftm.is_vrc7_channel(channel)) {
        uint8_t index = channel - ftm.vrc7_channel_index() + 1;
        buf[0] = 'F';
        buf[1] = 'M';
        buf[2] = (char)('0' + index);
        buf[3] = '\0';
        return buf;
    }
    if (ftm.is_vrc6_channel(channel)) {
        int first = ftm.vrc6_channel_index();
        if (channel == first + 2) {
            return "SAW";
        }
        uint8_t index = channel - first + 1;
        buf[0] = 'V';
        buf[1] = '6';
        buf[2] = (char)('0' + index);
        buf[3] = '\0';
        return buf;
    }
    if (ftm.is_mmc5_channel(channel)) {
        uint8_t index = channel - ftm.mmc5_channel_index() + 3;
        buf[0] = 'P';
        buf[1] = 'U';
        buf[2] = (char)('0' + index);
        buf[3] = '\0';
        return buf;
    }
    uint8_t index = channel > 99 ? 99 : channel;
    buf[0] = 'C';
    buf[1] = 'H';
    if (index >= 10) {
        buf[2] = (char)('0' + (index / 10));
        buf[3] = (char)('0' + (index % 10));
        buf[4] = '\0';
    } else {
        buf[2] = (char)('0' + index);
        buf[3] = '\0';
    }
    return buf;
}

static void visual_channel_values(uint8_t channel, uint8_t *pitchVal, uint8_t *volVal) {
    float period_rel = player.channel[channel].get_period_rel();
    if (channel == 2) {
        *pitchVal = (uint8_t)roundf(period2note(period_rel * 2.0f)) - 12;
        *volVal = player.channel[channel].get_rel_vol() ? 2 : 0;
        return;
    }

    *pitchVal = (uint8_t)roundf(period2note(period_rel)) - 12;
    uint8_t rel_vol = player.channel[channel].get_rel_vol();
    *volVal = (rel_vol + 55) / 56;
}

void osc_menu() {
    for (;;) {
        display.clearDisplay();
        display.setCursor(2, 0);
        display.print("R");
        uint8_t first_ch = visual_visible_first_channel();
        uint8_t visible_ch = visual_visible_channel_count();
        for (uint8_t d = 0; d < visible_ch; ++d) {
            uint8_t c = first_ch + d;
            display.setCursor(14 + (d * 24), 0);
            if (c < 5) {
                static const char *base_short[5] = {"PU1", "PU2", "TRI", "NOS", "DMC"};
                display.print(base_short[c]);
            } else if (ftm.is_fds_channel(c)) {
                display.print("FDS");
            } else if (ftm.is_vrc7_channel(c)) {
                display.printf("FM%d", c - ftm.vrc7_channel_index() + 1);
            } else if (ftm.is_vrc6_channel(c)) {
                int first = ftm.vrc6_channel_index();
                if (c == first + 2) display.print("SAW");
                else display.printf("V6%d", c - first + 1);
            } else if (ftm.is_mmc5_channel(c)) {
                display.printf("PU%d", c - ftm.mmc5_channel_index() + 3);
            } else {
                display.printf("CH%d", c);
            }
        }
        display.drawFastHLine(0, 6, 128, 1);
        display.setCursor(0, 11);
        display.drawFastHLine(0, 10, 128, 1);
        drawChessboard(0, 7, 7, 3);

        for (uint8_t d = 0; d < visible_ch; d++) {
            uint8_t c = first_ch + d;
            if (player.get_mute(c))
                drawChessboard((d * 24) + 8, 7, 23, 3);

            display.fillRect((d * 24) + 8, 7, roundf(player.channel[c].get_rel_vol() * (23.0f/225.0f)), 3, 1);
        }
        for (int r = -4; r < 5; r++) {
            if (r) {
                display.setTextColor(1);
            } else {
                display.fillRect(0, display.getCursorY() - 1, 7, 7, 1);
                display.setTextColor(0);
            }
            if (((player.get_row() + r) >= 0) && ((player.get_row() + r) < ftm.fr_block.pat_length)) {
                display.printf("%02X", player.get_row() + r);
            }
            display.printf("\n");
        }
        display.drawFastVLine(7, 0, 64, 1);
        for (uint8_t d = 0; d < visible_ch; d++) {
            uint8_t i = first_ch + d;
            display.drawFastVLine((d * 24) + 31, 0, 64, 1);
            if (i == channel_sel_pos) {
                display.setCursor((d * 24) + 9, 0);
                display.print(">");
                if (edit_mode) {
                    invertRect((d * 24) + 8, 0, 23, 6);
                }
            }
        }

        for (uint8_t d = 0; d < visible_ch; d++) {
            uint8_t i = first_ch + d;
            int draw_base = (d * 24) + 19;
            if (player.get_mute(i)) {
                drawPinstripe((d * 24) + 8, 11, 23, 53);
            }
            for (int y = 11; y < 63; y++) {
                int16_t p1 = limit_value(player.channel[i].get_buf()[y * 4] / 170, -12, 12);
                int16_t p2 = limit_value(player.channel[i].get_buf()[(y + 1) * 4] / 170, -12, 12);

                display.drawLine(draw_base + p2, y, draw_base + p1, y, 1);
            }
        }

        display.display();

        // KEYPAD
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (player.get_play_status()) {
                        pause_sound();
                    } else {
                        start_sound();
                    }
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

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }

        vTaskDelay(4);
    }
}


void visual_menu() {
    display.setTextColor(1);
    static ringbuf<uint8_t, 64> visual_buf[FAMI32_MAX_CHANNELS];
    static ringbuf<uint8_t, 64> visual_buf_vol[FAMI32_MAX_CHANNELS];
    static bool first_run = true;
    uint8_t channel_count = player.get_channel_count();
    if (channel_count > FAMI32_MAX_CHANNELS) {
        channel_count = FAMI32_MAX_CHANNELS;
    }

    if (first_run) {
        for (uint8_t c = 0; c < FAMI32_MAX_CHANNELS; ++c) {
            uint8_t pitchVal;
            uint8_t volVal;
            visual_channel_values(c, &pitchVal, &volVal);

            for (int i = 0; i < 64; ++i) {
                visual_buf[c].push(pitchVal);
                visual_buf_vol[c].push(volVal);
            }
        }
        first_run = false;
    }
    
    for (;;) {
        display.clearDisplay();

        channel_count = player.get_channel_count();
        if (channel_count > FAMI32_MAX_CHANNELS) {
            channel_count = FAMI32_MAX_CHANNELS;
        }

        static uint8_t pitchVals[FAMI32_MAX_CHANNELS];
        static uint8_t volVals[FAMI32_MAX_CHANNELS];
        
        for (uint8_t c = 0; c < channel_count; ++c) {
            visual_channel_values(c, &pitchVals[c], &volVals[c]);

            visual_buf[c].push(pitchVals[c]);
            visual_buf_vol[c].push(volVals[c]);

            uint8_t label_x = pitchVals[c] > 5 ? pitchVals[c] - 5 : 0;
            char label_buf[5];
            display.setCursor(label_x, 0);
            display.print(visual_channel_label(c, label_buf, sizeof(label_buf)));
        }

        for (int y = 0; y < 59; ++y) {
            int buf_index = 63 - y;
            for (uint8_t c = 0; c < channel_count; ++c) {
                uint8_t pitchHistory = visual_buf[c][buf_index];
                uint8_t volHistory = visual_buf_vol[c][buf_index];

                if (volHistory > 0) {
                    int draw_y = y + 5;
                    display.drawFastHLine(pitchHistory, draw_y, volHistory, 1);
                    int second_line_x = pitchHistory >= volHistory ? pitchHistory - volHistory + 1 : 0;
                    display.drawFastHLine(second_line_x, draw_y, volHistory, 1);
                }
            }
        }

        const char* title = ftm.nf_block.title;
        const char* author = ftm.nf_block.author;
        const char* copyright = ftm.nf_block.copyright;
        
        display.setCursor(0, 47);
        display.println(title);
        display.println(author);
        display.println(copyright);

        display.setCursor(90, 59);
        display.printf("(%02X/%02lX>%02X)", player.get_row(), ftm.fr_block.pat_length, player.get_frame());

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (player.get_play_status()) {
                        pause_sound();
                    } else {
                        start_sound();
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    if (edit_mode) {
                        unpk_item_t temp;
                        ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), temp);
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

        touch_input_event_t touch_event;
        if (touch_input_pop_event(&touch_event)) {
            process_note_io_event(note_io_event_from_input(touch_event));
        }
        vTaskDelay(3);
    }
}
