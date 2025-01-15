#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include <Adafruit_SSD1306.h>
#include "ftm_file.h"
#include "fonts/rismol_3_4.h"
#include "fonts/rismol_3_5.h"
#include "fonts/3x5font.h"

#include <dirent.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

extern FAMI_PLAYER player;
extern Adafruit_SSD1306 display;
extern Adafruit_Keypad keypad;

int main_menu_pos = 0;

void drawChessboard(int x, int y, int w, int h) {
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            display.drawPixel(x + col, y + row, (row + col) & 1);
        }
    }
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
        if (width > SCREEN_WIDTH - 20) width = SCREEN_WIDTH - 20;
    }

    if (height == 0) {
        height = numLines * textHeight + 4;
        if (height > SCREEN_HEIGHT - 20) height = SCREEN_HEIGHT - 20;
    }

    if (x == 0) {
        x = (SCREEN_WIDTH - width) / 2;
    }
    if (y == 0) {
        y = (SCREEN_HEIGHT - height) / 2;
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

int menu(const char* name, const char* menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void), uint16_t width, uint16_t height, uint16_t x, uint16_t y) {
    if (x == 0) x = (128 - width) / 2;
    if (y == 0) y = (64 - height) / 2;

    display.setTextColor(SSD1306_WHITE);

    int8_t menuPos = 0;
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

void menu_navi() {
    drawChessboard(0, 0, 128, 64);
    const char *menu_str[5] = {"MAIN", "FRAME", "CHANNEL", "INSTRUMENT", "OSC"};
    printf("MENU RETURN: %d\n", menu("MENU", menu_str, 5, NULL, 64, 45, 0, 0));
}

void open_file_page() {
    const char* file = file_sel("/flash");
    player.stop_play();
    drawPopupBox("READING...", 0, 0, 0, 0);
    vTaskDelay(16);
    ftm.open_ftm(file);
    ftm.read_ftm_all();
    player.reload();
}

void menu_file() {
    drawChessboard(0, 0, 128, 64);
    const char *menu_str[3] = {"OPEN", "SAVE", "RECORD"};
    int ret = menu("FILE", menu_str, 3, NULL, 64, 37, 0, 0);
    switch (ret)
    {
    case 0:
        open_file_page();
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
                            display.printf("===");
                        else
                            display.printf("%s%d ", note2str[pt_tmp.note], pt_tmp.octave);
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
        }
        display.display();

        // KEYPAD
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    if (player.get_play_status()) {
                        player.stop_play();
                    } else {
                        player.start_play();
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                } else if (e.bit.KEY == KEY_MENU) {
                    menu_file();
                }
            }
        }
        vTaskDelay(2);
    }
}

void channel_memu() {

}

void gui_task(void *arg) {
    for(;;) {
        switch (main_menu_pos)
        {
        case 0:
            tracker_menu();
            break;

        case 1:
        
        default:
            break;
        }
    }
}

#endif