#include "gui_input.h"

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

// Change selected channel (wrap around 0-4 for 5 channels total)
void set_channel_sel_pos(int8_t p) {
    if (p > 4) {
        p = 0;
    } else if (p < 0) {
        p = 4;
    }
    channel_sel_pos = p;
    // If in edit mode, ensure the player uses the currently selected instrument on the new channel
    if (edit_mode) {
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
    }
}

// Toggle edit mode (between view-only and edit) and ensure instrument selection is applied
void change_edit_mode() {
    edit_mode = !edit_mode;
    if (edit_mode) {
        player.channel[channel_sel_pos].set_inst(inst_sel_pos);
    }
}

// Pause keypad scanning until a key is pressed (used to debounce or wait for input)
void keypad_pause() {
    // Flush any existing event
    keypad.read();
    // Wait until a new event is available
    while (!keypad.available()) {
        vTaskDelay(32);
    }
    keypad.read();
}

// Display an on-screen keyboard for text input. Allows the user to input a string via the 16-key touchpad.
void displayKeyboard(const char *title, char *targetStr, uint8_t maxLen) {
    uint8_t cursorTick = 0;
    uint8_t charPos = strlen(targetStr);
    bool keyboardStat[16] = {0};
    char charOfst = 'A';       // Character offset (to allow switching between A-P, Q-Z etc.)
    bool cursorState = false;  // Blinking cursor state

    for (;;) {
        // Draw input UI
        display.clearDisplay();
        display.setFont(&rismol57);
        display.fillRect(0, 0, 128, 9, SSD1306_WHITE);
        display.setCursor(1, 1);
        display.setTextColor(SSD1306_BLACK);
        display.print(title);
        display.setTextColor(SSD1306_WHITE);
        display.setFont(NULL);  // use default font for input area

        // Text input area border
        display.drawRect(0, 16, 128, 11, SSD1306_WHITE);
        display.setCursor(2, 18);
        int16_t len = strlen(targetStr);
        if (len > 20) {
            // If text too long, show last 20 characters
            display.printf("%.20s", targetStr + len - 20);
        } else {
            display.printf("%s", targetStr);
        }
        // Blinking underscore cursor
        display.print(cursorState ? '_' : ' ');

        // Draw keyboard grid (2 rows x 8 columns)
        display.drawFastHLine(0, 43, 128, SSD1306_WHITE);
        display.drawFastHLine(0, 53, 128, SSD1306_WHITE);
        display.drawFastHLine(0, 63, 128, SSD1306_WHITE);
        for (uint8_t i = 0; i < 8; ++i) {
            display.drawFastVLine(i * 16, 44, 19, SSD1306_WHITE);
        }
        display.drawFastVLine(127, 44, 19, SSD1306_WHITE);
        // Draw letters A-P (or next set depending on charOfst)
        for (uint8_t c = 0; c < 8; ++c) {
            display.setTextColor(SSD1306_WHITE);
            display.setCursor((c * 16) + 6, 55);
            if (keyboardStat[c]) {
                display.fillRect(display.getCursorX() - 5, display.getCursorY() - 1, 15, 9, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            }
            display.printf("%c", c + charOfst);
        }
        for (uint8_t c = 8; c < 16; ++c) {
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(((c - 8) * 16) + 6, 45);
            if (keyboardStat[c]) {
                display.fillRect(display.getCursorX() - 5, display.getCursorY() - 1, 15, 9, SSD1306_WHITE);
                display.setTextColor(SSD1306_BLACK);
            }
            display.printf("%c", c + charOfst);
        }
        display.display();

        // Blink cursor timing
        cursorTick++;
        if (cursorTick > 64) {
            cursorTick = 0;
            cursorState = !cursorState;
        }

        // Handle keypad events for text input
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                if (e.bit.KEY == KEY_OCTU) {
                    // Next set of characters (e.g., from 'A' to 'Q')
                    charOfst += 16;
                    if (charOfst > 'Z') charOfst = 'A';
                } else if (e.bit.KEY == KEY_OCTD) {
                    // Previous set of characters
                    charOfst -= 16;
                    if (charOfst < 'A') charOfst = 'A';
                } else if (e.bit.KEY == KEY_S) {
                    // Delete (Backspace)
                    if (charPos > 0) {
                        charPos--;
                        targetStr[charPos] = '\0';
                    }
                } else if (e.bit.KEY == KEY_OK) {
                    // Confirm input
                    break;
                }
            }
        }
        // Handle touchpad events for letter input
        if (touchKeypad.available()) {
            touchKeypadEvent e = touchKeypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) {
                // Append character corresponding to touched key
                targetStr[charPos] = e.bit.KEY + charOfst;
                charPos++;
                if (charPos > maxLen) {
                    charPos--;  // prevent overflow
                }
                targetStr[charPos] = '\0';
                keyboardStat[e.bit.KEY] = true;
            } else if (e.bit.EVENT == KEY_JUST_RELEASED) {
                keyboardStat[e.bit.KEY] = false;
            }
        }
        vTaskDelay(4);
    }
    // Restore default font after exiting
    display.setFont(&rismol35);
}

// Test function to demonstrate the on-screen keyboard. It opens the keyboard, prints the result, and waits for any key press.
void test_displayKeyboard() {
    char testStr[32] = {0};
    displayKeyboard("TEST KEYBOARD", testStr, 31);
    ESP_LOGI("TEST_KEYBOARD", "Input string: %s", testStr);
    // Display the result on screen
    display.clearDisplay();
    display.fillRect(0, 0, 128, 9, SSD1306_WHITE);
    display.setFont(&rismol57);
    display.setCursor(1, 1);
    display.setTextColor(SSD1306_BLACK);
    display.print("TEST KEYBOARD");
    display.setFont(&rismol35);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 12);
    display.setTextWrap(true);
    display.printf("Return! testStr:\n%s\n", testStr);
    display.setTextWrap(false);
    display.display();
    // Wait for any key press to continue
    while (true) {
        if (keypad.available()) {
            keypadEvent e = keypad.read();
            if (e.bit.EVENT == KEY_JUST_PRESSED) break;
        }
        vTaskDelay(64);
    }
}
