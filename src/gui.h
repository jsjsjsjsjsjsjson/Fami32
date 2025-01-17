#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include <Adafruit_SSD1306.h>
#include "ftm_file.h"
#include "fonts/rismol_3_4.h"
#include "fonts/rismol_3_5.h"
#include "fonts/3x5font.h"
#include "fonts/font5x7.h"
#include "fonts/font4x6.h"

#include <fami32_icon.h>

#include <dirent.h>

extern FAMI_PLAYER player;
extern Adafruit_SSD1306 display;
extern Adafruit_Keypad keypad;
extern MPR121_Keypad touchKeypad;

extern TaskHandle_t SOUND_TASK_HD;
extern bool sound_task_stat;
extern i2s_chan_handle_t tx_handle;

static uint8_t main_menu_pos = 0;

static int8_t channel_sel_pos = 0;
static int16_t inst_sel_pos = 0;

uint8_t g_octv = 3;

bool touch_note = true;

bool edit_mode = false;

const char ch_name[5][10] = {
    "PULSE1", "PULSE2", "TRIANGLE", "NOISE", "DPCM"
};

void set_channel_sel_pos(int8_t p) {
    if (p > 4) {
        p = 0;
    } else if (p < 0) {
        p = 4;
    }
    channel_sel_pos = p;
    if (edit_mode)
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
}

void change_edit_mode() {
    edit_mode = !edit_mode;
    if (edit_mode)
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
}

void invertRect(int x, int y, int w, int h) {
    for (int i = x; i < x + w; i++) {
        for (int j = y; j < y + h; j++) {
            display.drawPixel(i, j, !display.getPixel(i, j));
        }
    }
}

void drawChessboard(int x, int y, int w, int h) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            display.drawPixel(x + col, y + row, (row + col) & 1);
        }
    }
}

void drawPinstript(int x, int y, int w, int h) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            display.drawPixel(x + col, y + row, !((row + col) & 3));
        }
    }
}

void pause_sound() {
    player.stop_play();
    size_t writed;
    int timer = 0;
    while (sound_task_stat) {
        timer++;
        if (timer > 64) {
            break;
        }
        vTaskDelay(1);
    }
    memset(player.get_buf(), 0, player.get_buf_size_byte());
    i2s_channel_write(tx_handle, player.get_buf(), player.get_buf_size_byte(), &writed, portMAX_DELAY);
}

void start_sound() {
    player.start_play();
}

void drawPopupBox(const char* message, uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    uint8_t textWidth = 0;
    uint8_t textHeight = 8;

    const int max_chars_per_line = 20;
    int numLines = 0;
    char* lines[10];
    char message_copy[strlen(message) + 1];
    strncpy(message_copy, message, sizeof(message_copy) - 1);
    message_copy[sizeof(message_copy) - 1] = '\0';

    char* token = strtok(message_copy, "\n");
    while (token != NULL && numLines < 10) {
        while (strlen(token) > max_chars_per_line) {
            int splitPos = max_chars_per_line;
            while (splitPos > 0 && token[splitPos] != ' ') {
                splitPos--;
            }
            if (splitPos == 0) splitPos = max_chars_per_line;

            char* line = (char*)malloc(splitPos + 1);
            strncpy(line, token, splitPos);
            line[splitPos] = '\0';
            lines[numLines++] = line;

            token += splitPos;
            while (*token == ' ') token++;
        }

        if (strlen(token) > 0 && numLines < 10) {
            lines[numLines++] = strdup(token);
        }

        token = strtok(NULL, "\n");
    }

    if (width == 0) {
        for (int i = 0; i < numLines; i++) {
            int len = strlen(lines[i]);
            if (len > textWidth) textWidth = len;
        }
        width = textWidth * 6 + 4;
        if (width > 128 - 20) width = 128 - 20;
    }

    if (height == 0) {
        height = numLines * textHeight + 4;
        if (height > 64 - 20) height = 64 - 20;
    }

    if (x == 0) {
        x = (128 - width) / 2;
    }
    if (y == 0) {
        y = (64 - height) / 2;
    }

    display.fillRect(x, y, width, height, 0);
    display.drawRect(x, y, width, height, 1);

    int totalTextHeight = numLines * textHeight;
    int startY = y + (height - totalTextHeight) / 2;

    for (int i = 0; i < numLines && i < 10; i++) {
        int lineLen = strlen(lines[i]);
        int textPixelWidth = lineLen * 6;
        int startX = x + (width - textPixelWidth) / 2;
        if (startX < x + 2) startX = x + 2;

        display.setCursor(startX, startY + i * textHeight);
        display.print(lines[i]);

        free(lines[i]);
    }

    display.display();
}

const char* file_sel(const char *basepath) {
    struct dirent **namelist = NULL;
    int n = 0;

    static char current_path[PATH_MAX];
    strncpy(current_path, basepath, PATH_MAX - 1);
    current_path[PATH_MAX - 1] = '\0';

    static char selected_file[PATH_MAX];

    int selected = 0;
    int top = 0;
    const int display_lines = 8;
    bool directory_changed = true;

    while (1) {
        if (directory_changed) {
            if (namelist != NULL) {
                for (int i = 0; i < n; i++) {
                    free(namelist[i]);
                }
                free(namelist);
                namelist = NULL;
                n = 0;
            }

            DIR *dir = opendir(current_path);
            if (dir == NULL) {

                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(0, 0);
                display.print("ERROR OPENING DIRECTORY:");
                display.setCursor(0, 8);
                display.print(current_path);
                display.display();
                sleep(2);
                return NULL;
            }

            n = scandir(current_path, &namelist, NULL, alphasort);
            closedir(dir);

            if (n < 0) {
                display.clearDisplay();
                display.setTextSize(1);
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(0, 0);
                display.print("ERROR READING DIRECTORY");
                display.display();
                sleep(2);
                return NULL;
            }

            selected = 0;
            top = 0;
            directory_changed = false;
        }

        if (selected < top) {
            top = selected;
        } else if (selected >= top + display_lines) {
            top = selected - display_lines + 1;
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print("SELECT A FILE IN:");
        display.setCursor(0, 7);
        display.print(current_path);
        display.drawFastHLine(0, 13, 128, SSD1306_WHITE);

        for (int i = top; i < top + display_lines && i < n; i++) {
            int displayIndex = i - top;
            if (i == selected) {
                display.fillRect(0, 15 + displayIndex * 6, 128, 7, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            } else {
                display.setTextColor(SSD1306_WHITE);
            }

            char *name = namelist[i]->d_name;

            char fullpath[PATH_MAX];
            snprintf(fullpath, PATH_MAX, "%s/%s", current_path, name);
            struct stat st;
            if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                char display_name[128];
                snprintf(display_name, sizeof(display_name), "%s/", name);
                display.setCursor(0, 16 + displayIndex * 6);
                display.print(display_name);
            } else {
                display.setCursor(0, 16 + displayIndex * 6);
                display.print(name);
            }

            if (i == selected) {
                display.setTextColor(SSD1306_WHITE);
            }
        }

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                switch (e.bit.KEY) {
                    case KEY_UP:
                        if (selected > 0) {
                            selected--;
                        } else {
                            selected = n - 1;
                        }
                        break;

                    case KEY_DOWN:
                        if (selected < n - 1) {
                            selected++;
                        } else {
                            selected = 0;
                        }
                        break;

                    case KEY_OK:
                        if (n <= 0) {
                            break;
                        } else {
                            char *selected_name = namelist[selected]->d_name;

                            char selected_path[PATH_MAX];
                            snprintf(selected_path, PATH_MAX, "%s/%s", current_path, selected_name);

                            struct stat st;
                            if (stat(selected_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                                strncpy(current_path, selected_path, PATH_MAX - 1);
                                current_path[PATH_MAX - 1] = '\0';
                                directory_changed = true;
                            } else {
                                strncpy(selected_file, selected_path, PATH_MAX - 1);
                                selected_file[PATH_MAX - 1] = '\0';

                                for (int i = 0; i < n; i++) {
                                    free(namelist[i]);
                                }
                                free(namelist);
                                namelist = NULL;
                                n = 0;

                                return selected_file;
                            }
                        }
                        break;

                    case KEY_BACK:
                        {
                            char *last_slash = strrchr(current_path, '/');
                            if (last_slash != NULL && last_slash != current_path) {
                                *last_slash = '\0';
                            } else {
                                strcpy(current_path, "/");
                            }
                            selected = 0;
                            top = 0;
                            directory_changed = true;
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        if (selected >= n) {
            selected = n - 1;
        }
    }
}

int menu(const char* name, const char* menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void), uint16_t width, uint16_t height, uint16_t x, uint16_t y, int8_t initPos) {
    if (x == 0) x = (128 - width) / 2;
    if (y == 0) y = (64 - height) / 2;

    display.setTextColor(1);

    int8_t menuPos = initPos;
    int8_t pageStart = 0;
    const uint8_t itemsPerPage = (height - 10) / 8;


    for (;;) {
        display.fillRect(x, y, width, height, 0);
        display.drawRect(x, y, width, height, 1);

        display.fillRect(x + 1, y + 1, width - 2, 8, 1);
        display.setTextColor(0);
        display.setCursor(x + 3, y + 2);
        display.print(name);
        display.setTextColor(1);

        if (menuPos < pageStart) {
            pageStart = menuPos;
        } else if (menuPos >= pageStart + itemsPerPage) {
            pageStart = menuPos - itemsPerPage + 1;
        }

        uint8_t pageEnd = (pageStart + itemsPerPage > maxMenuPos + 1) ? maxMenuPos + 1 : pageStart + itemsPerPage;

        for (uint8_t i = pageStart; i < pageEnd; i++) {
            uint8_t displayIndex = i - pageStart;
            uint16_t itemYPos = y + 10 + (displayIndex * 8);

            if (i == menuPos) {
                display.fillRect(x, itemYPos + 1, width, 7, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
                display.setCursor(x + 3, itemYPos + 2);
            } else {
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(x + 3, itemYPos + 2);
            }

            if (i >= maxMenuPos) {
                display.print("EXIT");
            } else {
                display.print(menuStr[i]);
            }
        }

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (menuPos >= maxMenuPos) {
                        return -1;
                    }
                    if (menuFunc != nullptr && menuFunc[menuPos] != nullptr) {
                        menuFunc[menuPos]();
                    } else {
                        return menuPos;
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    return -1;
                } else if (e.bit.KEY == KEY_UP) {
                    menuPos--;
                    if (menuPos < 0) {
                        menuPos = maxMenuPos;
                        pageStart = (maxMenuPos / itemsPerPage) * itemsPerPage;
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    menuPos++;
                    if (menuPos > maxMenuPos) {
                        menuPos = 0;
                        pageStart = 0;
                    }
                }
            }
        }

        vTaskDelay(4);
    }
}

int num_set_menu_int(const char* name, int min, int max, int count, int *num, int x, int y, int width, int height) {
    if (x == 0) x = (128 - width) / 2;
    if (y == 0) y = (64 - height) / 2;
    float numCount = (float)(width - 4) / max;
    
    for (;;) {
        display.fillRect(x, y, width, height, 0);
        display.drawRect(x, y, width, height, 1);

        display.fillRect(x + 1, y + 1, width - 2, 8, 1);
        display.setTextColor(0);
        display.setCursor(x + 3, y + 2);
        display.print(name);
        display.setTextColor(1);

        display.setCursor(x + 2, y + 12);
        display.setFont(&font5x7);
        display.print(*num);
        display.setFont(&rismol35);

        display.drawRect(x + 2, (y + height) - 10, width - 4, 8, 1);
        display.fillRect(x + 2, (y + height) - 10, *num * numCount, 8, 1);

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_L || e.bit.KEY == KEY_S) {
                    *num -= count;
                    if (*num < min) {
                        *num = min;
                    }
                } else if (e.bit.KEY == KEY_R || e.bit.KEY == KEY_P) {
                    *num += count;
                    if (*num > max) {
                        *num = max;
                    }
                } else if (e.bit.KEY == KEY_UP) {
                    *num += count * 2;
                    if (*num > max) {
                        *num = max;
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    *num -= count * 2;
                    if (*num < min) {
                        *num = min;
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    break;
                }
            }
        }

        vTaskDelay(4);
    }
    return *num;
}

void menu_navi() {
    drawPinstript(0, 0, 128, 64);
    const char *menu_str[5] = {"TRACKER", "CHANNEL", "FRAMES", "INSTRUMENT", "OSC"};
    int ret = menu("MENU", menu_str, 5, NULL, 64, 45, 0, 0, main_menu_pos);
    if (ret != -1)
        main_menu_pos = ret;
}

void open_file_page() {
    pause_sound();
    const char* file = file_sel("/flash");
    drawPopupBox("READING...", 0, 0, 0, 0);
    vTaskDelay(16);
    ftm.open_ftm(file);
    ftm.read_ftm_all();
    player.reload();
}

void channel_sel_page() {
    const char *menu_str[5] = {"PULSE1", "PULSE2", "TRIANGLE", "NOISE", "DPCM"};
    int ret = menu("CHANNEL", menu_str, 5, NULL, 64, 53, 0, 0, channel_sel_pos);
    if (ret != -1)
        set_channel_sel_pos(ret);
}

void menu_file() {
    // drawPinstript(0, 0, 128, 64);
    const char *menu_str[5] = {"NEW", "OPEN", "SAVE", "SAVE AS", "RECORD"};
    int ret = menu("FILE", menu_str, 5, NULL, 64, 45, 0, 0, 0);
    switch (ret)
    {
    case 0:
        pause_sound();
        ftm.new_ftm();
        player.reload();
        break;

    case 1:
        open_file_page();
        break;
    
    default:
        break;
    }
}

void update_touchpad_note(uint8_t *note, uint8_t *octv, touchKeypadEvent e) {
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
        if (e.bit.KEY > 13) {
            *note = e.bit.KEY - 1;
            *octv = 0;
        } else {
            *note = (e.bit.KEY % 12) + 1;
            *octv = g_octv + (e.bit.KEY / 12);
            if (touch_note) {
                player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                player.channel[channel_sel_pos].set_note(item2note(*note, *octv));
                player.channel[channel_sel_pos].note_start();
            }
        }
    } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        if (touch_note) {
            player.channel[channel_sel_pos].note_end();
        }
    }
}

void channel_setting_page() {
    drawPinstript(0, 0, 128, 64);
    const char *menu_str[2] = {"SELECT CHAN", "EXT EFX NUM"};
    char title[10];
    snprintf(title, 10, "CHANNEL%d", channel_sel_pos);
    int ret = menu(title, menu_str, 2, NULL, 64, 29, 0, 0, 0);
    int tmp = ftm.he_block.ch_fx[channel_sel_pos];
    switch (ret)
    {
    case 0:
        channel_sel_page();
        break;

    case 1:
        drawPinstript(0, 0, 128, 64);
        num_set_menu_int("EXT EFX NUM", 0, 3, 1, &tmp, 0, 0, 64, 32);
        ftm.he_block.ch_fx[channel_sel_pos] = tmp;
        break;

    default:
        break;
    }
}

void main_option_page() {
    drawPinstript(0, 0, 128, 64);
    const char *menu_str[5] = {edit_mode ? "STOP EDIT" : "START EDIT", 
                                player.get_mute(channel_sel_pos) ? "UNMUTE" : "MUTE",
                                "CHANNEL", "FILE", "SETTINGS"};
    int ret = menu("OPTION", menu_str, 5, NULL, 64, 45, 0, 0, 0);
    switch (ret)
    {
    case 0:
        edit_mode = !edit_mode;
        break;

    case 1:
        player.set_mute(channel_sel_pos, !player.get_mute(channel_sel_pos));
        break;
    
    case 2:
        channel_setting_page();
        break;

    case 3:
        menu_file();
        break;

    default:
        break;
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
                for (int i = 0; i < 5; i++) {
                    unpk_item_t pt_tmp = ftm.get_pt_item(i, player.get_cur_frame_map(i), player.get_row() + r);
                    if (pt_tmp.note != NO_NOTE) {
                        if (pt_tmp.note == NOTE_CUT)
                            display.printf("--- ");
                        else if (pt_tmp.note == NOTE_END)
                            display.printf("=== ");
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
            touchKeypadEvent e = touchKeypad.read();

            update_touchpad_note(&note_set, &octv_set, e);
            if (note_set) {
                printf("INPUT_NOTE_SET: %d\n", note_set);
                if (edit_mode) {
                    unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                    pt_tmp.note = note_set;
                    pt_tmp.octave = octv_set;
                    ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                    player.set_row(player.get_row() + 1);
                }
            }
        }

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
                } else if (e.bit.KEY == KEY_BACK) {
                    if (edit_mode) {
                        unpk_item_t pt_tmp;
                        ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
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
                }
            }
        }
        vTaskDelay(2);
    }
}

void channel_menu() {
    static uint8_t x_pos = 0;
    display.setFont(&font3x5);
    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&font5x7);
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

        display.display();

        uint8_t note_set = 0;
        uint8_t octv_set = 0;

        if (edit_mode) {
            if (touchKeypad.available()) {
                touchKeypadEvent e = touchKeypad.read();
                if (x_pos == 0) {
                    update_touchpad_note(&note_set, &octv_set, e);
                    if (note_set) {
                        printf("INPUT_NOTE_SET: %d\n", note_set);
                        unpk_item_t pt_tmp = ftm.get_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row());
                        pt_tmp.note = note_set;
                        if (pt_tmp.note > 12) {
                            pt_tmp.octave = 0;
                        } else {
                            pt_tmp.octave = octv_set;
                            pt_tmp.instrument = inst_sel_pos;
                        }
                        ftm.set_pt_item(channel_sel_pos, player.get_cur_frame_map(channel_sel_pos), player.get_row(), pt_tmp);
                        player.set_row(player.get_row() + 1);
                    }
                } else if (e.bit.EVENT == KEY_JUST_PRESSED) {
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
                            pt_tmp.fxdata[(x_pos - 4) / 3].fx_cmd = e.bit.KEY;
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
                    if (player.get_play_status()) {
                        pause_sound();
                    } else {
                        start_sound();
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
                        drawPinstript(0, 0, 128, 64);
                        channel_sel_page();
                    }
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
                    x_pos--;
                } else if (e.bit.KEY == KEY_R) {
                    x_pos++;
                }
                x_pos %= 4 + (ftm.ch_fx_count(channel_sel_pos) * 3);
            }
        }

        vTaskDelay(2);
    }
}

void frames_option_page() {
    drawPinstript(0, 0, 128, 64);
    const char *menu_str[5] = {"PUSH NEW", "INSERT NEW", "MOVE UP", "MOVE DOWN", "REMOVE"};
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
        display.setFont(&font5x7);
        display.setCursor(1, 1);
        display.printf("FRAMES ");
        display.setFont(&font4x6);
        display.printf("(%02X/%02X)", player.get_frame(), ftm.fr_block.frame_num - 1);

        int tol_vol = 0;
        for (int i = 0; i < 5; i++) {
            tol_vol += player.channel[i].get_rel_vol();
        }
        display.drawFastHLine(0, 9, tol_vol / 8, 1);

        display.setFont(&font5x7);
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
                display.setFont(&font5x7);
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
                }
            }
        }

        vTaskDelay(4);
    }
}

void instrument_menu() {
    int pageStart = 0;
    const int itemsPerPage = 6;
    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&font5x7);
        display.setCursor(1, 1);
        display.printf("INSTRUMENT ");
        display.setFont(&font4x6);
        display.printf("(%02X/%02X)", inst_sel_pos, ftm.inst_block.inst_num - 1);

        display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);

        display.setFont(&rismol35);
        display.setTextColor(1);

        if (inst_sel_pos < pageStart) {
            pageStart = inst_sel_pos;
        } else if (inst_sel_pos >= pageStart + itemsPerPage) {
            pageStart = inst_sel_pos - itemsPerPage + 1;
        }
        int pageEnd = (pageStart + itemsPerPage >= ftm.inst_block.inst_num) ? ftm.inst_block.inst_num : pageStart + itemsPerPage;

        for (uint8_t i = pageStart; i < pageEnd; i++) {
            int displayIndex = i - pageStart;
            int itemYPos = 9 + (displayIndex * 8);

            if (i == inst_sel_pos) {
                display.fillRect(0, itemYPos + 1, 128, 7, 1);
                display.setTextColor(0);
                display.setCursor(2, itemYPos + 2);
            } else {
                display.setTextColor(1);
                display.setCursor(2, itemYPos + 2);
            }

            if (ftm.get_inst(i)->name_len > 18) {
                display.printf("%02X>%.15s...", ftm.get_inst(i)->index, ftm.get_inst(i)->name);
            } else {
                display.printf("%02X>%s", ftm.get_inst(i)->index, ftm.get_inst(i)->name);
            }

            for (int f = 0; f < 5; f++) {
                if (i == inst_sel_pos)
                    display.drawBitmap(88 + (f * 8), itemYPos + 1, fami32_icon[f], 7, 7, !ftm.get_inst(i)->seq_index[f].enable);
                else
                    display.drawBitmap(88 + (f * 8), itemYPos + 1, fami32_icon[f], 7, 7, ftm.get_inst(i)->seq_index[f].enable);
            }
        }
        display.setTextColor(1);

        display.drawFastVLine(86, 10, 47, 1);
        display.drawFastHLine(0, 57, 128, 1);

        display.drawFastVLine(63, 58, 6, 1);
        display.setCursor(0, 59);
        display.printf("CH%d: %.10s", channel_sel_pos, player.channel[channel_sel_pos].get_inst()->name);

        if (channel_sel_pos == 3)
            display.drawFastVLine(63 + (player.channel[3].get_noise_rate() * 4), 58, 6, 1);
        else if (channel_sel_pos == 4) 
            display.drawFastVLine(63 + (player.channel[4].get_samp_pos() * (64.0f / (float)player.channel[4].get_samp_len())), 58, 6, 1);
        else
            display.drawFastVLine(63 + (64 - (player.channel[channel_sel_pos].get_period() / 32)), 58, 6, 1);

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    // OK
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_BACK) {
                    // BACK
                } else if (e.bit.KEY == KEY_UP) {
                    inst_sel_pos--;
                    if (inst_sel_pos < 0) {
                        inst_sel_pos = ftm.inst_block.inst_num - 1;
                        pageStart = (ftm.inst_block.inst_num / itemsPerPage) * itemsPerPage;
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    inst_sel_pos++;
                    if (inst_sel_pos >= ftm.inst_block.inst_num) {
                        inst_sel_pos = 0;
                        pageStart = 0;
                    }
                }
            }
        }

        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            uint8_t note_set, octv_set;
            update_touchpad_note(&note_set, &octv_set, e);
        }

        vTaskDelay(4);
    }
}

void gui_task(void *arg) {
    player.channel[channel_sel_pos].set_inst(inst_sel_pos);
    for(;;) {
        switch (main_menu_pos)
        {
        case 0:
            tracker_menu();
            break;

        case 1:
            channel_menu();
            break;

        case 2:
            frames_menu();
            break;

        case 3:
            instrument_menu();
            break;
        
        default:
            break;
        }
    }
}

#endif