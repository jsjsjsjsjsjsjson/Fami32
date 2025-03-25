#include "gui_visual.h"
#include "gui_menu.h"      // for drawChessboard, drawPinstripe
#include "gui_tracker.h"   // for menu_navi(), main_option_page()
#include "gui_input.h"

void osc_menu() {
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

        for (int c = 0; c < 5; c++) {
            if (player.get_mute(c))
                drawChessboard((c * 24) + 8, 7, 23, 3);

            display.fillRect((c * 24) + 8, 7, roundf(player.channel[c].get_rel_vol() * (23.0f/225.0f)), 3, 1);
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

        for (int i = 0; i < 5; i++) {
            int draw_base = (i * 24) + 19;
            // display.drawFastVLine((i * 24) + 19, 11, 53, 1);
            if (player.get_mute(i)) {
                drawPinstripe((i * 24) + 8, 11, 23, 53);
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

        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(NULL, NULL, e);
        }

        vTaskDelay(4);
    }
}

void visual_menu() {
    display.setTextColor(SSD1306_WHITE);
    // Use ring buffers to store history of pitch and volume values for each channel
    static ringbuf<uint8_t, 128> visual_buf[5];
    static ringbuf<uint8_t, 128> visual_buf_vol[5];
    static const char chn_labels[5][4] = {"PU1", "PU2", "TRI", "NOS", "DMC"};
    for (;;) {
        display.clearDisplay();
        // Update buffers with current values for each channel
        for (int c = 0; c < 5; ++c) {
            uint8_t pitchVal;
            uint8_t volVal;
            if (c == 2) {  // Triangle channel
                pitchVal = (uint8_t)roundf(period2note(player.channel[c].get_period_rel() * 2)) - 12;
                volVal = player.channel[c].get_rel_vol() ? 2 : 0;
            } else {
                pitchVal = (uint8_t)roundf(period2note(player.channel[c].get_period_rel())) - 12;
                volVal = (player.channel[c].get_rel_vol() + 55) / 56;
            }
            visual_buf[c].push(pitchVal);
            visual_buf_vol[c].push(volVal);
            // Draw channel label at its current pitch position
            display.setCursor(pitchVal > 5 ? pitchVal - 5 : 0, 0);
            display.print(chn_labels[c]);
        }
        // Draw the history graph (pitch vs volume) for each channel
        for (int y = 0; y < 123; ++y) {
            for (int c = 0; c < 5; ++c) {
                uint8_t pitchHistory = visual_buf[c][127 - y];
                uint8_t volHistory = visual_buf_vol[c][127 - y];
                // Draw horizontal lines representing volume (two lines to make a thicker point)
                display.drawFastHLine(pitchHistory, y + 5, volHistory, SSD1306_WHITE);
                display.drawFastHLine(pitchHistory - volHistory + 1, y + 5, volHistory, SSD1306_WHITE);
            }
        }
        // Display song info at bottom
        display.setCursor(0, 47);
        display.println(ftm.nf_block.title);
        display.println(ftm.nf_block.author);
        display.println(ftm.nf_block.copyright);
        // Display current row/frame info at bottom-right
        display.setCursor(90, 59);
        display.printf("(%02X/%02lX>%02X)", player.get_row(), ftm.fr_block.pat_length, player.get_frame());
        display.display();

        // Handle keypad input (similar to oscilloscope)
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
                        // If in edit mode, clear copy mode data on BACK
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
        // Consume touch events (if any) to avoid stuck notes
        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(nullptr, nullptr, e);
        }
        vTaskDelay(4);
    }
}
