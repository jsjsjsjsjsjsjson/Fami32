#ifndef GUI_COMMON_H
#define GUI_COMMON_H

// Common includes and global definitions for the GUI system.
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <cstdlib>

// Platform/Framework specific includes (ESP32 and device drivers)
#include <esp_log.h>
#include <esp_system.h>
#include <driver/i2s_std.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Keypad.h>
#include <MPR121_Keypad.h>
#include <USBMIDI.h>
#include "ringbuf.h"
#include "fami32_pin.h"
#include "fami32_icon.h"
#include "env_gen.h"
#include "ftm_file.h"
#include "fami32_player.h"
extern "C" {
#include "micro_config.h"
}

// Font includes (font objects used for display text)
#include "fonts/rismol_3_4.h"
#include "fonts/rismol_3_5.h"
#include "fonts/rismol_5_7.h"
#include "fonts/font4x6.h"

extern FAMI_PLAYER player;
extern Adafruit_SSD1306 display;
extern Adafruit_Keypad keypad;
extern MPR121_Keypad touchKeypad;

extern i2s_chan_handle_t tx_handle;

// Global GUI state variables (extern definitions, actual instances in gui_common.cpp)
extern uint8_t main_menu_pos;
extern int8_t channel_sel_pos;
extern int inst_sel_pos;
extern uint8_t g_octv;
extern int g_vol;
extern bool touch_note;
extern bool edit_mode;
extern bool setting_change;
extern int copy_start;
extern bool copy_mode;
extern bool _midi_output;
extern const char *config_path;  // Path to configuration file for settings

// Global data structures
extern std::vector<unpk_item_t> clipboard_data;  // Clipboard for copied pattern data
extern const char ch_name[5][10];                // Names of channels (for display)

// Common utility functions available to all modules
void pause_sound();
void start_sound();
void drawPopupBox(const char* message);

// Declaration of GUI task loop (to be run in main or OS task)
void gui_task(void *arg);

#endif // GUI_COMMON_H
