#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include <vector>
#include <esp_log.h>
#include <Adafruit_SSD1306.h>
#include "ftm_file.h"
#include "fonts/rismol_3_4.h"
#include "fonts/rismol_3_5.h"
#include "fonts/3x5font.h"
#include "fonts/rismol_5_7.h"
#include "fonts/font4x6.h"
#include <USBMIDI.h>

#include "ringbuf.h"

extern "C" {
#include "micro_config.h"
}

#include "fami32_pin.h"

#include <fami32_icon.h>

#include <sys/stat.h>
#include <dirent.h>

const char *random_name[177] = {
    "Brave", "Zephyr", "Glade", "Frost", "Echo", "Briar", "Thorn", "Alder",
    "Misty", "Flare", "Nimbus", "Gale", "Sylva", "Cliff", "Raven", "Ashen",
    "Vail", "Drift", "Skye", "Dawn", "Coral", "Storm", "Ember", "Warden",
    "Pine", "Flint", "Violet", "Cinder", "Onyx", "Blaze", "Thorny", "Horizon",
    "Quill", "Violet", "Noble", "Fjord", "Cobalt", "Ashby", "Peregrine", "Opal",
    "Rustle", "Falcon", "Vermil", "Ivy", "Haven", "Slate", "Talon", "Tempest",
    "Lynx", "Ridge", "Shade", "Astra", "Vesper", "Nash", "Sable", "Drake",
    "Rook", "Moss", "Emberly", "Hail", "Cedar", "Mist", "Birch", "Crimson",
    "Starla", "Dusk", "Wilder", "Ashwood", "Jade", "Ravenna", "Breeze", "Nira",
    "Omen", "Fable", "Vale", "Thistle", "Severn", "Glimmer", "Sablewood", "Wisp",
    "Bracken", "Emberstone", "Winterfell", "Dawnstone", "Blightmoor", "Ironbark", 
    "Frostbloom", "Shadowmere", "Nightfall", "Mooncrest", "Windhaven", "Stormwatch", 
    "Falconer", "Mistral", "Silverthorn", "Dragonfire", "Vortex", "Ravenclaw", 
    "Draegar", "Sablewind", "Grimwood", "Valkyrie", "Darkholme", "Stoneforge",
    "Seabrook", "Celestia", "Ashfall", "Hallowbrook", "Thundershade", "Winterhold",
    "Obsidian", "Stormbringer", "Nightshade", "Frostveil", "Ironfang", "Moonshadow",
    "Brightstone", "Griffon", "Halloway", "Redthorn", "Brimstone", "Wolfbane",
    "Oakencrest", "Ironveil", "Swiftwind", "Bluefire", "Crystal", "Duskwalker", 
    "Mistveil", "Goldleaf", "Ravenwood", "Briarwood", "Falconcrest", "Stormbringer", 
    "Thornfield", "Nighthawk", "Vandale", "Silverclaw", "Eldritch", "Tornel", 
    "Glimmerstone", "Wolfsbane", "Ebonfall", "Blightforge", "Nightwing", "Duskfall",
    "Ravencroft", "Thorncrest", "Falcontide", "Vampira", "Firestrike", "Glacier",
    "Snowcrest", "Havenglen", "Stoneheart", "Mistrider", "Darkwing", "Ashburn", 
    "Silverfang", "Draugr", "Emberfall", "Stormrage", "Moonfire", "Obsidian", 
    "Ironclad", "Mistvale", "Thunderpeak", "Dreadmoor", "Skyfire", "Blazewood", 
    "Ravenshade", "Brumel", "Shadewood", "Frostmoon", "Valkyr", "Wyrmstone", "Dragonhold"
};

extern FAMI_PLAYER player;
extern Adafruit_SSD1306 display;
extern Adafruit_Keypad keypad;
extern MPR121_Keypad touchKeypad;

extern TaskHandle_t SOUND_TASK_HD;
extern bool sound_task_stat;
extern i2s_chan_handle_t tx_handle;

extern bool _midi_output;

extern USBMIDI MIDI;

static uint8_t main_menu_pos = 0;

static int8_t channel_sel_pos = 0;
static int inst_sel_pos = 0;

uint8_t g_octv = 3;

extern int g_vol;

bool touch_note = true;

extern bool edit_mode;

bool setting_change = false;

int copy_start = 0;
// int copy_end = 0;
bool copy_mode = false;

std::vector<unpk_item_t> clipboard_data;

const char ch_name[5][10] = {
    "PULSE1", "PULSE2", "TRIANGLE", "NOISE", "DPCM"
};

int limit_value(int value, int min, int max) {
    return (value < min) ? min : (value > max) ? max : value;
}

void keypad_pause() {
    keypad.read();
    while (!keypad.available()) {
        vTaskDelay(32);
    }
    keypad.read();
}

void displayKeyboard(const char *title, char *targetStr, uint8_t maxLen) {
    uint8_t curserTick = 0;
    uint8_t charPos = strlen(targetStr);
    bool keyboardStat[16] = {0};
    char charOfst = 'A';
    bool curserStat = false;
    for (;;) {
        display.clearDisplay();
        display.setFont(&rismol57);
        display.fillRect(0, 0, 128, 9, 1);
        display.setCursor(1, 1);
        display.setTextColor(0);
        display.print(title);
        display.setTextColor(1);
        // display.setCursor(0, 12);
        display.setFont(NULL);
        display.drawRect(0, 16, 128, 11, 1);
        display.setCursor(2, 18);
        int16_t len = strlen(targetStr);
        if (len > 20) {
            display.printf("%.20s", targetStr + len - 20);
        } else {
            display.printf("%s", targetStr);
        }
        display.print(curserStat ? '_' : ' ');
        display.drawFastHLine(0, 43, 128, 1);
        display.drawFastHLine(0, 53, 128, 1);
        display.drawFastHLine(0, 63, 128, 1);
        for (uint8_t i = 0; i < 8; i++) {
            display.drawFastVLine(i * 16, 44, 19, 1);
        }
        display.drawFastVLine(127, 44, 19, 1);
        for (uint8_t c = 0; c < 8; c++) {
            display.setTextColor(1);
            display.setCursor((c * 16) + 6, 55);
            if (keyboardStat[c]) {
                display.fillRect(display.getCursorX() - 5, display.getCursorY() - 1, 15, 9, 1);
                display.setTextColor(0);
            }
            display.printf("%c", c + charOfst);
        }
        for (uint8_t c = 8; c < 16; c++) {
            display.setTextColor(1);
            display.setCursor(((c - 8) * 16) + 6, 45);
            if (keyboardStat[c]) {
                display.fillRect(display.getCursorX() - 5, display.getCursorY() - 1, 15, 9, 1);
                display.setTextColor(0);
            }
            display.printf("%c", c + charOfst);
        }
        display.display();
        curserTick++;
        if (curserTick > 64) {
            curserTick = 0;
            curserStat = !curserStat;
        }
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OCTU) {
                    charOfst += 16;
                } else if (e.bit.KEY == KEY_OCTD) {
                    charOfst -= 16;
                } else if (e.bit.KEY == KEY_S) {
                    if (charPos > 0) {
                        charPos--;
                        targetStr[charPos] = '\0';
                    }
                } else if (e.bit.KEY == KEY_OK) {
                    break;
                }
            }
        }
        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                targetStr[charPos] = e.bit.KEY + charOfst;
                charPos++;
                if (charPos > maxLen) {
                    charPos--;
                }
                targetStr[charPos] = '\0';
                keyboardStat[e.bit.KEY] = true;
            } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
                keyboardStat[e.bit.KEY] = false;
            }
        }
        vTaskDelay(4);
    }
    display.setFont(&rismol35);
}

void set_channel_sel_pos(int8_t p) {
    if (p > 4) {
        p = 0;
    } else if (p < 0) {
        p = 4;
    }
    // printf("SET_CHAN: %d\n", p);
    channel_sel_pos = p;
    if (edit_mode)
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
}

void update_touchpad_note(uint8_t *note, uint8_t *octv, touchKeypadEvent e) {
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
        if (e.bit.KEY > 13) {
            if (note != NULL) *note = e.bit.KEY - 1;
            if (octv != NULL) *octv = 0;
        } else {
            if (note != NULL) *note = (e.bit.KEY % 12) + 1;
            if (octv != NULL) *octv = g_octv + (e.bit.KEY / 12);
            if (touch_note) {
                player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                player.channel[channel_sel_pos].set_note(item2note((e.bit.KEY % 12) + 1, g_octv + (e.bit.KEY / 12)));
                player.channel[channel_sel_pos].note_start();
            }
            if (_midi_output) {
                MIDI.noteOn(item2note((e.bit.KEY % 12) + 1, g_octv + (e.bit.KEY / 12)), 120, channel_sel_pos);
            }
        }
    } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
        if (touch_note) {
            player.channel[channel_sel_pos].note_end();
            if (_midi_output) {
                MIDI.noteOff(item2note((e.bit.KEY % 12) + 1, g_octv + (e.bit.KEY / 12)), 120, channel_sel_pos);
            }
        }
    }
}

void update_midi_note(uint8_t *note, uint8_t *octv, uint8_t *vol) {
    midiEventData_t e;
    MIDI.readPacket((midiEventPacket_t*)&e);
    if (e.cin == MIDI_CIN_NOTE_ON) {
        if (note != NULL) *note = (e.note % 12) + 1;
        if (octv != NULL) *octv = (e.note / 12) - 2;
        if (vol != NULL) *vol = e.vol >> 3;
    } else if (e.cin == MIDI_CIN_NOTE_OFF) {
        if (player.get_play_status()) {
            if (note != NULL) *note = 14;
            if (octv != NULL) *octv = 0;
        }
    }
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

void drawRectWithNegativeSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w < 0) {
        x += w;
        w = -w;
    }

    if (h < 0) {
        y += h;
        h = -h;
    }
    display.drawRect(x, y, w, h, color);
}

void fillRectWithNegativeSize(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w < 0) {
        x += w;
        w = -w;
    }

    if (h < 0) {
        y += h;
        h = -h;
    } else {
        y += 1;
    }
    display.fillRect(x, y, w, h, color);
}

void drawChessboardWithNegativeSize(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (w < 0) {
        x += w;
        w = -w;
    }

    if (h < 0) {
        y += h;
        h = -h;
    } else {
        y += 1;
    }
    drawChessboard(x, y, w, h);
}

void pause_sound() {
    player.stop_play();
    size_t writed;
    // int timer = 0;
    // while (sound_task_stat) {
    //     timer++;
    //     if (timer > 64) {
    //         break;
    //     }
    //     vTaskDelay(1);
    // }
    if (_midi_output) {
        for (int i = 0; i < 5; i++ ) {
            MIDI.noteOff(0, 0, i);
        }
    }
    memset(player.get_buf(), 0, player.get_buf_size_byte());
    i2s_channel_write(tx_handle, player.get_buf(), player.get_buf_size_byte(), &writed, portMAX_DELAY);
}

void start_sound() {
    player.start_play();
}

void drawPopupBox(const char* message) {
    display.setTextWrap(false);
    display.setTextColor(1);

    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(message, 0, 0, &x1, &y1, &textWidth, &textHeight);

    int boxWidth = textWidth + 4;
    int boxHeight = textHeight + 4;

    int boxX = (128 - boxWidth) / 2;
    int boxY = (64 - boxHeight) / 2;

    display.fillRect(boxX + 1, boxY + 1, boxWidth - 2, boxHeight - 2, 0);
    display.drawRect(boxX - 1, boxY - 1, boxWidth + 2, boxHeight + 2, 0);
    display.drawRect(boxX, boxY, boxWidth, boxHeight, 1);

    int textX = boxX + 2;
    int textY = boxY + 2;

    display.setCursor(textX, textY);
    display.print(message);

    display.display();
}

const char* file_sel(const char *basepath) {
    display.setFont(&rismol35);
    struct dirent **namelist = NULL;
    int n = 0;

    static char current_path[PATH_MAX];
    strncpy(current_path, basepath, PATH_MAX - 1);
    current_path[PATH_MAX - 1] = '\0';
    static char fullpath[1280];
    static char selected_file[PATH_MAX];

    int selected = 0;
    int top = 0;
    const int display_lines = 8;
    bool directory_changed = true;

    static struct {
        char name[256];
        bool is_dir;
    } dir_cache[256];
    int dir_cache_size = 0;

    display.setTextColor(0);
    display.fillRect(0, 0, 128, 6, 1);
    display.setCursor(0, 0);
    display.print("FILE SELECTOR");
    display.setTextColor(1);

    for(;;) {
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
                vTaskDelay(1024);
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
                vTaskDelay(1024);
                return NULL;
            }

            dir_cache_size = 0;
            for (int i = 0; i < n && dir_cache_size < 256; i++) {
                snprintf(dir_cache[dir_cache_size].name, sizeof(dir_cache[dir_cache_size].name), "%s", namelist[i]->d_name);
                snprintf(fullpath, sizeof(fullpath), "%s/%s", current_path, namelist[i]->d_name);
                struct stat st;
                dir_cache[dir_cache_size].is_dir = (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode));
                dir_cache_size++;
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

        display.fillRect(0, 6, 128, 121, 0);
        display.setCursor(0, 7);
        display.printf(":%s", current_path);
        display.drawFastHLine(0, 13, 128, SSD1306_WHITE);

        for (int i = top; i < top + display_lines && i < dir_cache_size; i++) {
            int displayIndex = i - top;
            if (i == selected) {
                display.fillRect(0, 15 + displayIndex * 6, 128, 7, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            } else {
                display.setTextColor(SSD1306_WHITE);
            }

            if (dir_cache[i].is_dir) {
                static char display_name[512];
                snprintf(display_name, sizeof(display_name), "%s/", dir_cache[i].name);
                display.setCursor(0, 16 + displayIndex * 6);
                display.print(display_name);
            } else {
                display.setCursor(0, 16 + displayIndex * 6);
                display.print(dir_cache[i].name);
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
                            selected = dir_cache_size - 1;
                        }
                        break;

                    case KEY_DOWN:
                        if (selected < dir_cache_size - 1) {
                            selected++;
                        } else {
                            selected = 0;
                        }
                        break;

                    case KEY_OK:
                        if (dir_cache_size <= 0) {
                            break;
                        } else {
                            char *selected_name = dir_cache[selected].name;

                            static char selected_path[PATH_MAX];
                            snprintf(selected_path, PATH_MAX - 1, "%s/%s", current_path, selected_name);

                            if (dir_cache[selected].is_dir) {
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

        if (selected >= dir_cache_size) {
            selected = dir_cache_size - 1;
        }

        vTaskDelay(1);
    }
}

int menu(const char* name, const char* menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void), uint16_t width, uint16_t height, uint16_t x = 0, uint16_t y = 0, int8_t initPos = 0, int *menuVar = NULL) {
    if (x == 0) x = (128 - width) / 2;
    if (y == 0) y = (64 - height) / 2;

    display.setTextColor(1);

    int8_t menuPos = initPos;
    int8_t pageStart = 0;
    const uint8_t itemsPerPage = (height - 10) / 8;

    display.drawRect(x - 1, y - 1, width + 2, height + 2, 0);

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
                display.setCursor(x + 4, itemYPos + 2);
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
                    if (menuVar != NULL) {
                        *menuVar = menuPos;
                    }
                } else if (e.bit.KEY == KEY_DOWN) {
                    menuPos++;
                    if (menuPos > maxMenuPos) {
                        menuPos = 0;
                        pageStart = 0;
                    }
                    if (menuVar != NULL) {
                        *menuVar = menuPos;
                    }
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

int fullScreenMenu(const char* name, const char* menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void), int8_t initPos) {
    display.setTextColor(1);

    int8_t menuPos = initPos;
    int8_t pageStart = 0;
    const uint8_t itemsPerPage = 6;

    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setFont(&rismol57);
        display.setTextColor(0);
        display.setCursor(1, 1);
        display.print(name);
        display.setTextColor(1);
        display.setFont(&rismol35);

        if (menuPos < pageStart) {
            pageStart = menuPos;
        } else if (menuPos >= pageStart + itemsPerPage) {
            pageStart = menuPos - itemsPerPage + 1;
        }

        uint8_t pageEnd = (pageStart + itemsPerPage > maxMenuPos + 1) ? maxMenuPos + 1 : pageStart + itemsPerPage;

        for (uint8_t i = pageStart; i < pageEnd; i++) {
            uint8_t displayIndex = i - pageStart;
            uint16_t itemYPos = 10 + (displayIndex * 8);

            if (i == menuPos) {
                display.fillRect(0, itemYPos + 1, 128, 7, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
                display.setCursor(2, itemYPos + 2);
            } else {
                display.setTextColor(SSD1306_WHITE);
                display.setCursor(1, itemYPos + 2);
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
    float numCount = (float)(width - 4) / (max - min);

    display.drawRect(x - 1, y - 1, width + 2, height + 2, 0);
    
    for (;;) {
        display.fillRect(x, y, width, height, 0);
        display.drawRect(x, y, width, height, 1);

        display.fillRect(x + 1, y + 1, width - 2, 8, 1);
        display.setTextColor(0);
        display.setCursor(x + 3, y + 2);
        display.print(name);
        display.setTextColor(1);

        display.setCursor(x + 2, y + 12);
        display.setFont(&rismol57);
        display.print(*num);
        display.setFont(&rismol35);

        display.drawRect(x + 2, (y + height) - 10, width - 4, 8, 1);
        display.fillRect(x + 2, (y + height) - 10, (*num - min) * numCount, 8, 1);

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
                } else if (e.bit.KEY == KEY_BACK || e.bit.KEY == KEY_OK) {
                    break;
                }
            }
        }

        vTaskDelay(4);
    }
    return *num;
}

void vol_set_page() {
    int vol_last = g_vol;
    num_set_menu_int("VOLUME", 0, 64, 1, &g_vol, 0, 0, 68, 32);
    if (g_vol != vol_last) {
        drawPopupBox("SAVE CONFIG...");
        display.display();
        set_config_value("VOLUME", CONFIG_INT, &g_vol);
        write_config(config_path);
    }
}

void menu_navi() {
    drawPinstript(0, 0, 128, 64);
    static const char *menu_str[7] = {"TRACKER", "CHANNEL", "FRAMES", "INSTRUMENT", "INFO & SETTING", "OSCILLOSCOPE", "VISUALIZATION"};
    int ret = menu("MENU", menu_str, 7, NULL, 64, 45, 0, 0, main_menu_pos);
    if (ret != -1)
        main_menu_pos = ret;
}

void open_file_page() {
    drawPopupBox("LOADING...");
    pause_sound();
    const char* file = file_sel("/flash");
    drawPopupBox("READING...");
    int ret = ftm.open_ftm(file);
    ESP_LOGI("OPEN_FTM", "Open file %s...", file);
    if (ret) {
        switch (ret)
        {
        case FILE_OPEN_ERROR:
            drawPopupBox("OPEN FILE ERROR");
            ESP_LOGE("OPEN_FTM", "OPEN FILE %s ERROR", file);
            break;

        case FTM_NOT_VALID:
            drawPopupBox("NOT A VALID FTM FILE");
            ESP_LOGE("OPEN_FTM", "%s NOT A VALID FTM FILE", file);
            break;

        case NO_SUPPORT_VERSION:
            drawPopupBox("UNSUPPORTED .FTM VERSION");
            ESP_LOGE("OPEN_FTM", "UNSUPPORTED %s VERSION", file);
            break;
        }
        display.display();
        ftm.new_ftm();
        vTaskDelay(1024);
        return;
    }
    ret = ftm.read_ftm_all();
    ESP_LOGI("OPEN_FTM", "READING FTM...");
    if (ret) {
        switch (ret)
        {
        case NO_SUPPORT_EXTCHIP:
            drawPopupBox("EXT-CHIP NOT SUPPORTED");
            ESP_LOGE("OPEN_FTM", "EXT-CHIP NOT SUPPORTED");
            break;

        case NO_SUPPORT_MULTITRACK:
            drawPopupBox("MULTI-TRACK NOT SUPPORTED");
            ESP_LOGE("OPEN_FTM", "MULTI-TRACK NOT SUPPORTED");
            break;
        
        default:
            break;
        }
        display.display();
        ftm.new_ftm();
        vTaskDelay(1024);
        return;
    }
    player.reload();
}

void channel_sel_page() {
    static const char *menu_str[5] = {"PULSE1", "PULSE2", "TRIANGLE", "NOISE", "DPCM"};
    int ret = menu("CHANNEL", menu_str, 5, NULL, 64, 53, 0, 0, channel_sel_pos);
    if (ret != -1)
        set_channel_sel_pos(ret);
}

void menu_file() {
    // drawPinstript(0, 0, 128, 64);
    static const char *menu_str[5] = {"NEW", "OPEN", "SAVE", "SAVE AS", "RECORD"};
    int ret = menu("FILE", menu_str, 5, NULL, 64, 45, 0, 0, 0);
    static char current_file[256];
    static char target_file[256];
    switch (ret)
    {
    case 0:
        inst_sel_pos = 0;
        pause_sound();
        ftm.new_ftm();
        player.reload();
        break;

    case 1:
        inst_sel_pos = 0;
        open_file_page();
        break;

    case 2:
        if (strlen(ftm.current_file) == 0) {
            strcpy(current_file, "Untitled");
            displayKeyboard("SAVE...", current_file, 255);
            printf("CUR: %s\n", current_file);
            snprintf(target_file, 254, "/flash/%s.ftm", current_file);
            printf("SAVE AS TO: %s\n", target_file);
            drawPopupBox("WRITING...");
            display.display();
            ftm.save_as_ftm(target_file);
        } else {
            drawPopupBox("WRITING...");
            display.display();
            ftm.save_ftm();
        }
        break;

    case 3:
        strcpy(current_file, basename(ftm.current_file));
        current_file[strlen(current_file) - 4] = '\0';
        displayKeyboard("SAVE AS...", current_file, 255);
        printf("CUR: %s\n", current_file);
        snprintf(target_file, 254, "/flash/%s.ftm", current_file);
        printf("SAVE AS TO: %s\n", target_file);
        drawPopupBox("WRITING...");
        display.display();
        ftm.save_as_ftm(target_file);
        break;
    
    default:
        break;
    }
}

void channel_setting_page() {
    drawPinstript(0, 0, 128, 64);
    static const char *menu_str[2] = {"SELECT CHAN", "EXT EFX NUM"};
    char title[10];
    snprintf(title, 14, "CHANNEL%d", channel_sel_pos);
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

void device_setting() {

}

void samp_rate_set() {
    num_set_menu_int("SAMPLE RATE", 10800, 192000, 6000, &SAMP_RATE, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("SAMPLE_RATE", CONFIG_INT, &SAMP_RATE) == CONFIG_SUCCESS) {
        printf("Updated 'SAMPLE_RATE' to %d\n", SAMP_RATE);
    }
}

void eng_speed_set() {
    num_set_menu_int("ENGINE SPEED", 1, 240, 1, &ENG_SPEED, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("ENGINE_SPEED", CONFIG_INT, &ENG_SPEED) == CONFIG_SUCCESS) {
        printf("Updated 'ENGINE_SPEED' to %d\n", ENG_SPEED);
    }
}

void low_pass_set() {
    num_set_menu_int("LOW PASS", 1000, SAMP_RATE / 2, 1000, &LPF_CUTOFF, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("LPF_CUTOFF", CONFIG_INT, &LPF_CUTOFF) == CONFIG_SUCCESS) {
        printf("Updated 'LPF_CUTOFF' to %d\n", LPF_CUTOFF);
    }
}

void high_pass_set() {
    num_set_menu_int("HIGH PASS", 0, 1000, 10, &HPF_CUTOFF, 0, 0, 128, 32);
    setting_change = true;
    if (set_config_value("HPF_CUTOFF", CONFIG_INT, &HPF_CUTOFF) == CONFIG_SUCCESS) {
        printf("Updated 'HPF_CUTOFF' to %d\n", HPF_CUTOFF);
    }
}

void finetune_set() {
    num_set_menu_int("FINETUNE", 400, 500, 1, &BASE_FREQ_HZ, 0, 0, 128, 32);
    if (set_config_value("BASE_FREQ_HZ", CONFIG_INT, &BASE_FREQ_HZ) == CONFIG_SUCCESS) {
        printf("Updated 'BASE_FREQ_HZ' to %d\n", BASE_FREQ_HZ);
    }
}

void erase_config_set() {
    static const char *menu_str[2] = {"NO", "YES"};
    int ret = menu("ARE YOU SURE !?", menu_str, 2, NULL, 68, 29, 0, 0, 0);
    if (ret == 1) {
        display.setFont(&rismol57);
        drawPopupBox("ERASE CONFIG...");
        display.display();
        remove(config_path);
        esp_restart();
    }
}

void over_sample_set() {
    num_set_menu_int("OVER SAMPLE", 1, 8, 1, &OVER_SAMPLE, 0, 0, 100, 32);
    if (set_config_value("OVER_SAMPLE", CONFIG_INT, &OVER_SAMPLE) == CONFIG_SUCCESS) {
        printf("Updated 'OVER_SAMPLE' to %d\n", OVER_SAMPLE);
    }
}

void midi_out_set() {
    static const char *menu_str[2] = {"OFF", "ON"};
    int r = menu("MIDI OUT", menu_str, 2, NULL, 60, 29, 0, 0, _midi_output);
    if (r != -1) {
        _midi_output = r;
    }
}

void settings_page() {
    static const char *menu_str[9] = {"SAMPLE RATE", "ENGINE SPEED", "LOW PASS", "HIGH PASS", "FINETUNE", "OVER SAMPLE", "VOLUME", "MIDI OUT", "RESET CONFIG"};
    void (*menuFunc[9])(void) = {samp_rate_set, eng_speed_set, low_pass_set, high_pass_set, finetune_set, over_sample_set, vol_set_page, midi_out_set, erase_config_set};
    fullScreenMenu("SETTINGS", menu_str, 9, menuFunc, 0);
    // display.setFont(&rismol57);
    drawPopupBox("SAVE CONFIG...");
    display.display();
    write_config(config_path);
    if (setting_change) {
        drawPopupBox("Requires reboot to apply changes.");
        display.display();
        vTaskDelay(1024);
        esp_restart();
    }
    display.setFont(&rismol35);
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

void test_displayKeyboard() {
    char testStr[32] = {0};
    displayKeyboard("TEST KEYBOARD", testStr, 31);
    printf("Return! testStr:\n%s\n", testStr);
    display.clearDisplay();
    display.fillRect(0, 0, 128, 9, 1);
    display.setFont(&rismol57);
    display.setCursor(1, 1);
    display.setTextColor(0);
    display.printf("TEST KEYBOARD");
    display.setFont(&rismol35);
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.setTextWrap(true);
    display.printf("Return! testStr:\n%s\n", testStr);
    display.setTextWrap(false);
    display.display();
    for (;;) {
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) break;
        }
        vTaskDelay(64);
    }
}

void reboot_page() {
    static const char *menu_str[2] = {"NO", "YES"};
    int ret = menu("REBOOT?", menu_str, 2, NULL, 42, 29, 0, 0, 0);
    if (ret == 1) {
        display.setFont(&rismol57);
        drawPopupBox("REBOOTING...");
        display.display();
        esp_restart();
    }
}

void copy_data(int start, int end, int channel) {
    clipboard_data.resize(end - start);
    for (int i = start; i < end; i++) {
        clipboard_data[i - start] = ftm.get_pt_item(channel, player.get_cur_frame_map(channel), i);
    }
}

void paste_data(int start, int channel) {
    if (edit_mode) {
        for (int i = 0; i < clipboard_data.size(); i++) {
            if ((i + start) < ftm.unpack_pt[channel][player.get_cur_frame_map(channel)].size()) {
                ftm.set_pt_item(channel, player.get_cur_frame_map(channel), i + start, clipboard_data[i]);
            }
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

void main_option_page() {
    drawPinstript(0, 0, 128, 64);
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
            uint8_t vol_set = NO_VOL;

            if (touchKeypad.available()) {
                touchKeypadEvent e = touchKeypad.read();
                update_touchpad_note(&note_set, &octv_set, e);
            }

            if (note_set) {
                printf("INPUT_NOTE_SET: %d\n", note_set);
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
                        drawPinstript(0, 0, 128, 64);
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

void frames_option_page() {
    drawPinstript(0, 0, 128, 64);
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

void sequence_editor(instrument_t *inst) {
    int sequ_type = 0;

    int sequ_sel_index = 0;

    int pageStart = 0;
    int pageEnd = 32;

    static const char *sequ_name[5] = {"VOLUME", "ARPEGGIO", "PITCH", "HI-PITCH", "DUTY"};

    static const char *menu_str[4] = {"LENGTH", "LOOP", "RELEASE", "SEQU INDEX"};

    int draw_base = 30;

    if (inst == NULL) {
        return;
    }

    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setCursor(1, 1);
        display.setFont(&rismol57);
        display.print(inst->name);

        display.drawBitmap(120, 1, fami32_icon[sequ_type], 7, 7, 0);
        if (!inst->seq_index[sequ_type].enable) {
            invertRect(119, 0, 9, 9);
        }

        // display.drawFastHLine(0, 9, player.channel[channel_sel_pos].get_rel_vol() / 2, 1);

        // display.drawRect(0, 10, 128, 42, 1);
        display.drawFastHLine(0, 10, 128, 1);
        display.drawFastHLine(0, 51, 128, 1);

        display.setFont(&rismol35);
        display.setTextColor(1);

        sequences_t *sequ = ftm.get_sequ(sequ_type, inst->seq_index[sequ_type].seq_index);

        if (sequ != NULL) {
            int draw_w = (sequ->length <= 32) ? (128 / sequ->length) : 4;

            if (sequ->length <= 32) {
                pageEnd = sequ->length;
                drawChessboard(sequ->length * draw_w, 11, 128 - (sequ->length * draw_w), 40);
            }

            display.setFont(&rismol34);
            display.setCursor(112, 54);
            display.print("LOOP");
            display.setCursor(100, 59);
            display.print("RELEASE");
            display.setFont(&rismol35);

            if (pageEnd > sequ->loop) {
                int draw_start = sequ->loop - pageStart;
                invertRect(draw_start * draw_w, 54, 128 - (draw_start * draw_w), 4);
            }

            if (pageEnd > sequ->release) {
                int draw_start = sequ->release - pageStart;
                invertRect(draw_start * draw_w, 59, 128 - (draw_start * draw_w), 4);
            }

            if (sequ_type == VOL_SEQU) {
                drawChessboard(0, 20, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (51 - (sequ->data[i] * 2)) + 1, draw_w - 2, (sequ->data[i] * 2) - 2);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (51 - (sequ->data[i] * 2)), draw_w, sequ->data[i] * 2, 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (51 - (sequ->data[i] * 2)), draw_w, sequ->data[i] * 2, 1);
                    }
                }
                display.setCursor(0, 12);
            } else if (sequ_type == ARP_SEQU) {
                drawChessboard(0, draw_base, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    // display.drawRect(i * draw_w, (51 - (sequ->data[i] * 2)), draw_w, sequ->data[i] * 2, 1);
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (draw_base - sequ->data[i]), draw_w - 1, 1);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (draw_base - sequ->data[i]) - 1, draw_w + 1, 3, 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (draw_base - sequ->data[i]) - 1, draw_w + 1, 3, 1);
                    }
                }
                display.setCursor(0, 45);
            } else if (sequ_type == DTY_SEQU) {
                drawChessboard(0, 20, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboard((displayIndex * draw_w) + 1, (51 - (sequ->data[i] * 10)) + 1, draw_w - 2, (sequ->data[i] * 10) - 2);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        display.fillRect(displayIndex * draw_w, (51 - (sequ->data[i] * 10)), draw_w, (sequ->data[i] * 10), 1);
                    } else {
                        display.drawRect(displayIndex * draw_w, (51 - (sequ->data[i] * 10)), draw_w, (sequ->data[i] * 10), 1);
                    }
                }
                display.setCursor(0, 12);
            } else {
                drawChessboard(0, draw_base, 128, 1);
                for (int i = pageStart; i < pageEnd; i++) {
                    int displayIndex = i - pageStart;
                    if ((player.channel[channel_sel_pos].get_inst() == inst) && (player.channel[channel_sel_pos].get_inst_pos(sequ_type) == i)) {
                        drawChessboardWithNegativeSize((displayIndex * draw_w) + 1, draw_base, draw_w - 2, -sequ->data[i]);
                    }
                    if (i == sequ_sel_index) {
                        display.drawFastHLine(displayIndex * draw_w, 12, draw_w, 1);
                        fillRectWithNegativeSize(displayIndex * draw_w, draw_base, draw_w, -sequ->data[i], 1);
                    } else {
                        drawRectWithNegativeSize(displayIndex * draw_w, draw_base, draw_w, -sequ->data[i], 1);
                    }
                }
                display.setCursor(0, 45);
            }

            display.printf("(%d,%d)", sequ_sel_index, sequ->data[sequ_sel_index]);
        } else {
            display.setCursor(0, 14);
            display.printf("NULL %s SEQUENCE: #%d\nPRESS OK TO CREATE IT...", sequ_name[sequ_type], inst->seq_index[sequ_type].seq_index);
        }
        
        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    inst->seq_index[sequ_type].enable = !inst->seq_index[sequ_type].enable;
                    if (sequ == NULL) {
                        ftm.resize_sequ(sequ_type, inst->seq_index[sequ_type].seq_index + 1);
                        ftm.resize_sequ_len(sequ_type, inst->seq_index[sequ_type].seq_index, 1);
                    }
                } else if (e.bit.KEY == KEY_BACK) {
                    return;
                } else if (e.bit.KEY == KEY_MENU) {
                    int ret = menu("SELECT SEQUENCE", sequ_name, 5, NULL, 68, 53, 0, 0, sequ_type);
                    if (ret != -1) {
                        sequ_sel_index = 0;
                        pageEnd = 32;
                        pageStart = 0;
                        sequ_type = ret;
                    }
                } else if (e.bit.KEY == KEY_NAVI) {
                    int ret = menu("SEQUENCE", menu_str, 4, NULL, 60, 45, 0, 0, 0);
                    int sequ_len;
                    int sequ_index_ref;
                    int loop_ref;
                    int release_ref;
                    if (sequ != NULL) {
                        sequ_len = sequ->length;
                        sequ_index_ref = inst->seq_index[sequ_type].seq_index;
                        loop_ref = sequ->loop;
                        release_ref = sequ->release;
                    }
                    switch (ret)
                    {
                    case 0:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU LENGTH", 1, 255, 1, &sequ_len, 0, 0, 100, 34);
                            ftm.resize_sequ_len(sequ_type, inst->seq_index[sequ_type].seq_index, sequ_len);
                            sequ_sel_index = sequ_len - 1;
                        }
                        break;

                    case 1:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU LOOP", -1, sequ_len - 1, 1, &loop_ref, 0, 0, 126, 34);
                            sequ->loop = loop_ref;
                        }
                        break;

                    case 2:
                        if (sequ != NULL) {
                            num_set_menu_int("SEQU RELEASE", -1, sequ_len - 1, 1, &release_ref, 0, 0, 126, 34);
                            sequ->release = release_ref;
                        }
                        break;

                    case 3:
                        num_set_menu_int("SEQU INDEX", 0, 255, 1, &sequ_index_ref, 0, 0, 64, 34);
                        inst->seq_index[sequ_type].seq_index = sequ_index_ref;
                        break;

                    default:
                        break;
                    }
                } else if (e.bit.KEY == KEY_L) {
                    if (sequ != NULL) {
                        sequ_sel_index--;
                        if (sequ_sel_index < 0) {
                            sequ_sel_index = sequ->length - 1;
                            if (sequ->length > 32) {
                                pageStart = sequ->length - 32;
                                pageEnd = sequ->length;
                            }
                        } else if (sequ_sel_index < pageStart) {
                            pageStart = sequ_sel_index;
                            pageEnd = pageStart + 32;
                        }
                    }
                } else if (e.bit.KEY == KEY_R) {
                    if (sequ != NULL) {
                        sequ_sel_index++;
                        if (sequ_sel_index >= sequ->length) {
                            sequ_sel_index = 0;
                            pageStart = 0;
                            pageEnd = 32;
                        } else if (sequ_sel_index >= pageEnd) {
                            pageEnd = sequ_sel_index + 1;
                            pageStart = pageEnd - 32;
                        }
                    }
                } else if (e.bit.KEY == KEY_UP) {
                    draw_base++;
                } else if (e.bit.KEY == KEY_DOWN) {
                    draw_base--;
                } else if (e.bit.KEY == KEY_P) {
                    if (sequ != NULL) {
                        sequ->data[sequ_sel_index]++;
                        if (sequ_type == VOL_SEQU && sequ->data[sequ_sel_index] > 15) {
                            sequ->data[sequ_sel_index] = 15;
                        } if (sequ_type == DTY_SEQU && sequ->data[sequ_sel_index] > 3) {
                            sequ->data[sequ_sel_index] = 0;
                        }
                    }
                } else if (e.bit.KEY == KEY_S) {
                    if (sequ != NULL) {
                        sequ->data[sequ_sel_index]--;
                        if (sequ->data[sequ_sel_index] < 0) {
                            if (sequ_type == VOL_SEQU) {
                                sequ->data[sequ_sel_index] = 0;
                            } else if (sequ_type == DTY_SEQU) {
                                sequ->data[sequ_sel_index] = 3;
                            }
                        }
                    }
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

void instrument_option_page() {
    static const char *menu_str[4] = {"NEW", "RENAME", "RAND NAME", "REMOVE"};
    srand(time(NULL));
    int ret = menu("INSTRUMENT", menu_str, 4, NULL, 60, 45, 0, 0, 0);
    switch (ret)
    {
    case 0:
        ftm.create_new_inst();
        break;

    case 1:
        displayKeyboard("INSTRUMENT NAME", ftm.get_inst(inst_sel_pos)->name, 63);
        ftm.get_inst(inst_sel_pos)->name_len = strlen(ftm.get_inst(inst_sel_pos)->name);
        break;

    case 2:
        strcpy(ftm.get_inst(inst_sel_pos)->name, random_name[rand() % 177]);
        ftm.get_inst(inst_sel_pos)->name_len = strlen(ftm.get_inst(inst_sel_pos)->name);
        break;

    case 3:
        ftm.remove_inst(inst_sel_pos);
        break;
    
    default:
        break;
    }
    if (inst_sel_pos >= ftm.inst_block.inst_num) {
        inst_sel_pos = ftm.inst_block.inst_num - 1;
    }
}

void instrument_menu() {
    if (inst_sel_pos >= ftm.inst_block.inst_num) {
        inst_sel_pos = ftm.inst_block.inst_num - 1;
    }
    static int pageStart = 0;
    const int itemsPerPage = 6;
    for (;;) {
        display.clearDisplay();

        display.fillRect(0, 0, 128, 9, 1);
        display.setTextColor(0);
        display.setFont(&rismol57);
        display.setCursor(1, 1);
        display.print("INSTRUMENT ");
        display.setFont(&font4x6);
        display.printf("(%02X/%02lX)", inst_sel_pos, ftm.inst_block.inst_num - 1);

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
                display.printf("%02lX>%.15s...", ftm.get_inst(i)->index, ftm.get_inst(i)->name);
            } else {
                display.printf("%02lX>%s", ftm.get_inst(i)->index, ftm.get_inst(i)->name);
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
            display.drawFastVLine(63 + roundf(period2note(player.channel[channel_sel_pos].get_period_rel()) * 0.5f), 58, 6, 1);

        display.display();

        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OK) {
                    sequence_editor(ftm.get_inst(inst_sel_pos));
                } else if (e.bit.KEY == KEY_NAVI) {
                    menu_navi();
                    break;
                } else if (e.bit.KEY == KEY_MENU) {
                    instrument_option_page();
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
                drawPinstript((i * 24) + 8, 11, 23, 53);
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

void visual_menu() {
    display.setTextColor(1);
    static const char chn[5][4] = {"PU1", "PU2", "TRI", "NOS", "DMC"};
    ringbuf<uint8_t, 128> visual_buf[5];
    ringbuf<uint8_t, 128> visual_buf_vol[5];
    for (;;) {
        display.clearDisplay();

        for (int c = 0; c < 5; c++) {
            uint8_t p;
            uint8_t v;
            if (c == 2) {
                p = roundf(period2note(player.channel[c].get_period_rel() * 2)) - 12;
                v = player.channel[c].get_rel_vol() ? 2 : 0;
            } else {
                p = roundf(period2note(player.channel[c].get_period_rel())) - 12;
                v = (player.channel[c].get_rel_vol() + 55) / 56;
            }
            visual_buf[c].push(p);
            visual_buf_vol[c].push(v);
            display.setCursor(p - 5, 0);
            display.print(chn[c]);
        }

        for (int y = 0; y < 123; y++) {
            for (int c = 0; c < 5; c++) {
                // display.drawPixel(visual_buf[c][127 - y], y, 1);
                display.drawFastHLine(visual_buf[c][127 - y], y + 5, visual_buf_vol[c][127 - y], 1);
                display.drawFastHLine(visual_buf[c][127 - y] - visual_buf_vol[c][127 - y] + 1, y + 5, visual_buf_vol[c][127 - y], 1);
            }
        }

        display.setCursor(0, 47);
        display.println(ftm.nf_block.title);
        display.println(ftm.nf_block.author);
        display.println(ftm.nf_block.copyright);

        display.setCursor(90, 59);
        display.printf("(%02X/%02lX>%02X)", player.get_row(), ftm.fr_block.pat_length, player.get_frame());

        display.display();

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

        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            update_touchpad_note(NULL, NULL, e);
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

        case 4:
            song_info_menu();
            break;
        
        case 5:
            osc_menu();
            break;

        case 6:
            visual_menu();
            break;

        default:
            break;
        }
    }
}

#endif