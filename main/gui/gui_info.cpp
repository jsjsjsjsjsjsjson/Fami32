#include "gui_info.h"
#include "gui_menu.h"      // for menu(), drawPinstripe
#include "gui_input.h"     // for displayKeyboard()
#include "gui_tracker.h"   // for menu_navi()

void song_info_menu() {
    int page = 0;
    int select = -1;

    static const char *info_menu_str[3] = {"TITLE", "AUTHOR", "COPYRIGHT"};

    static const char *setting_menu_str[3] = {"TEMPO", "SPEED", "ROWS"};

    static char *song_info[3] = {ftm.nf_block.title, ftm.nf_block.author, ftm.nf_block.copyright};

    static int *song_setting[3] = {(int*)&ftm.fr_block.tempo, (int*)&ftm.fr_block.speed, (int*)&ftm.fr_block.pat_length};
    static int min[3] = {32, 1, 1};
    static int max[3] = {255, 31, 256};

    for (;;) {
        display.clearDisplay();
        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print(page ? "SONG SETTING" : "SONG INFO");

        if (page) {
            for (int i = 0; i < 3; i++) {
                if (i == select) {
                    display.setTextColor(0);
                    display.fillRect(0, 11 + (i * 17), 128, 15, 1);
                } else {
                    display.setTextColor(1);
                }
                display.setFont(&rismol35);
                display.setCursor(1, 12 + (i * 17));
                display.printf("%s:", setting_menu_str[i]);
                display.setFont(&rismol57);
                display.setCursor(1, 18 + (i * 17));
                display.print(*song_setting[i]);
            }
        } else {
            for (int i = 0; i < 3; i++) {
                if (i == select) {
                    display.setTextColor(0);
                    display.fillRect(0, 11 + (i * 17), 128, 15, 1);
                } else {
                    display.setTextColor(1);
                }
                display.setFont(&rismol35);
                display.setCursor(1, 12 + (i * 17));
                display.printf("%s:", info_menu_str[i]);
                display.setFont(&rismol57);
                display.setCursor(1, 18 + (i * 17));
                display.print(song_info[i]);
            }
        }

        display.display();

        // KEYPAD
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (select != -1) {
                        if (page) {
                            int last_row = ftm.fr_block.pat_length;
                            num_set_menu_int(setting_menu_str[select], min[select], max[select], 1, song_setting[select], 0, 0, 120, 36);
                            if (ftm.fr_block.pat_length != last_row) {
                                pause_sound();
                                player.set_row(0);
                                display.setFont(&rismol35);
                                drawPopupBox("RESIZE PATTERN...");
                                display.display();
                                for (int c = 0; c < ftm.pr_block.channel; c++) {
                                    for (int i = 0; i < ftm.unpack_pt[c].size(); i++) {
                                        ftm.unpack_pt[c][i].resize(ftm.fr_block.pat_length);
                                    }
                                }
                            }
                            player.ref_speed_and_tempo();
                        } else {
                            displayKeyboard(info_menu_str[select], song_info[select], 31);
                        }
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    display.setFont(&rismol35);
                    menu_navi();
                    break;
                }if (e.bit.KEY == KEY_UP) {
                    select--;
                    if (select < -1) select = 2;
                } else if (e.bit.KEY == KEY_DOWN) {
                    select++;
                    if (select > 2) select = 0;
                } else if ((e.bit.KEY == KEY_L) || (e.bit.KEY == KEY_R)) {
                    page = !page;
                    select = -1;
                }
            }
        }

        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(NULL, NULL, e);
        }

        vTaskDelay(16);
    }
}
