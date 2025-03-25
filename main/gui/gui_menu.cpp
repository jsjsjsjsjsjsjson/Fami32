#include "gui_menu.h"
#include "gui_input.h"  // for update_touchpad_note()

// Invert the pixels in a rectangle (useful for highlighting selection)
void invertRect(int x, int y, int w, int h) {
    for (int i = x; i < x + w; ++i) {
        for (int j = y; j < y + h; ++j) {
            display.drawPixel(i, j, !display.getPixel(i, j));
        }
    }
}

// Draw a chessboard (checkered) pattern in the given rectangle (used for shading regions or muted areas)
void drawChessboard(int x, int y, int w, int h) {
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            display.drawPixel(x + col, y + row, ((row + col) & 1) ? SSD1306_WHITE : SSD1306_BLACK);
        }
    }
}

// Draw a pinstripe pattern (diagonal stripes) in the given rectangle (used as background for menus, etc.)
void drawPinstripe(int x, int y, int w, int h) {
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
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

int menu(const char* name, const char* menuStr[], uint8_t maxMenuPos, void (*menuFunc[])(void), uint16_t width, uint16_t height, uint16_t x, uint16_t y, int8_t initPos, int *menuVar) {
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
                display.print("CANCEL");
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

// Render a full-screen menu (occupies entire OLED display). Returns selected index or 255 if cancelled.
int fullScreenMenu(const char* title, const char* options[], uint8_t optionCount, void (*menuActions[])(void), int8_t initPos) {
    // Leverage menu() with full screen dimensions
    return menu(title, options, optionCount, menuActions, 128, 64, 0, 0, initPos);
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

// The main navigation menu: allows user to choose major UI sections (Tracker, Channel, Frames, etc.)
void menu_navi() {
    drawPinstripe(0, 0, 128, 64);
    static const char *menu_items[7] = {
        "TRACKER", "CHANNEL", "FRAMES", "INSTRUMENT", "INFO & SETTING", "OSCILLOSCOPE", "VISUALIZATION"
    };
    int ret = menu("MENU", menu_items, 7, NULL, 64, 45, 0, 0, main_menu_pos);
    if (ret != -1) {
        // Update the selected menu index (state for gui_task)
        main_menu_pos = ret;
    }
}
