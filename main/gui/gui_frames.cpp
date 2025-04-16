#include "gui_frames.h"
#include "gui_menu.h"      // for menu(), drawPinstripe
#include "gui_input.h"     // for set_channel_sel_pos()
#include "gui_tracker.h"   // for menu_navi(), main_option_page() (called within frames_menu)

void frames_option_page() {
    drawPinstripe(0, 0, 128, 64);
    static const char *menu_str[5] = {"PUSH NEW", "INSERT NEW", "MOVE UP", "MOVE DOWN", "REMOVE"};
    char title[10];
    snprintf(title, 8, "FRAME%d", player.get_frame());
    int ret = menu(title, menu_str, 5, NULL, 64, 45, 0, 0, 0);
    switch (ret)
    {
    case 0:
        ftm.new_frame();
        break;

    case 1:
        ftm.insert_new_frame(player.get_frame());
        break;

    case 2:
        ftm.moveup_frame(player.get_frame());
        break;

    case 3:
        ftm.movedown_frame(player.get_frame());
        break;

    case 4:
        ftm.remove_frame(player.get_frame());
        break;
    
    default:
        break;
    }
}

void frames_menu() {
    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.printf("FRAMES ");
        display.setFont(&font4x6);
        display.printf("(%02X/%02lX)", player.get_frame(), ftm.fr_block.frame_num - 1);

        int tol_vol = 0;
        for (int i = 0; i < 5; i++) {
            tol_vol += player.channel[i].get_rel_vol();
        }
        display.drawFastHLine(0, 9, tol_vol / 8, 1);

        display.setFont(&rismol57);
        display.setCursor(1, 12);
        for (int f = -2; f < 3; f++) {
            if (f) {
                display.setTextColor(1);
            } else {
                display.setTextColor(0);
                display.fillRect(0, display.getCursorY() - 2, 90, 11, 1);
            }

            if (((player.get_frame() + f) >= 0) && ((player.get_frame() + f) < ftm.fr_block.frame_num)) {
                display.setFont(&font4x6);
                display.printf("%02X", player.get_frame() + f);
                display.setCursor(display.getCursorX() + 3, display.getCursorY());
                display.setFont(&rismol57);
                for (int c = 0; c < 5; c++) {
                    display.printf("%02X", ftm.get_frame_map(player.get_frame() + f, c));
                    display.setCursor(display.getCursorX() + 3, display.getCursorY());
                }
            }
            display.printf("\n");
            display.setCursor(display.getCursorX() + 1, display.getCursorY() + 3);
        }

        display.drawFastHLine(0, 10, 128, 1);
        display.drawFastVLine(12, 11, 53, 1);
        display.drawFastVLine(90, 10, 54, 1);
        display.drawFastVLine(91, 10, 54, 1);
        display.drawFastVLine(92, 10, 54, 1);

        invertRect(15 + (channel_sel_pos * 15), 33, 13, 9);

        display.setFont(&rismol35);

        display.setCursor(95, 11);

        for (int r = 0; r < 9; r++) {
            if (r == (player.get_row() % 9)) {
                display.fillRect(93, 10 + (r * 6), 35, 7, 1);
                display.setTextColor(0);
            } else {
                display.setTextColor(1);
            }
            unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos),
                                                r + ((player.get_row() / 9) * 9));
            if (pt_tmp.note != NO_NOTE) {
                if (pt_tmp.note == NOTE_CUT)
                    display.printf("CUT ");
                else if (pt_tmp.note == NOTE_END)
                    display.printf("RLS ");
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
            display.printf("\n");
            display.setCursor(95, display.getCursorY());
        }

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
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {
                    frames_option_page();
                } else if (e.bit.KEY == KEY_P) {
                    ftm.set_frame_plus1(player.get_frame(), channel_sel_pos);
                } else if (e.bit.KEY == KEY_S) {
                    ftm.set_frame_minus1(player.get_frame(), channel_sel_pos);
                } else if (e.bit.KEY == KEY_UP) {
                    player.jmp_to_frame(player.get_frame() - 1);
                    player.set_row(player.get_row() + 1);
                } else if (e.bit.KEY == KEY_DOWN) {
                    player.jmp_to_frame(player.get_frame() + 1);
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
