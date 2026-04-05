#ifndef GUI_INPUT_H
#define GUI_INPUT_H

#include "gui_common.h"
#include "note_io.h"
#include "touch_input.h"

// Input handling and on-screen keyboard interface
note_io_event_t note_io_event_from_input(const touch_input_event_t &e);
void set_channel_sel_pos(int8_t p);
void change_edit_mode();
void keypad_pause();
void displayKeyboard(const char *title, char *targetStr, uint8_t maxLen);
void test_displayKeyboard();

#endif // GUI_INPUT_H
