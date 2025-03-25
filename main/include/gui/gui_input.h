#ifndef GUI_INPUT_H
#define GUI_INPUT_H

#include "gui_common.h"

// Input handling and on-screen keyboard interface
void update_touchpad_note(uint8_t *note, uint8_t *octv, touchKeypadEvent e);
void set_channel_sel_pos(int8_t p);
void change_edit_mode();
void keypad_pause();
void displayKeyboard(const char *title, char *targetStr, uint8_t maxLen);
void test_displayKeyboard();

#endif // GUI_INPUT_H
