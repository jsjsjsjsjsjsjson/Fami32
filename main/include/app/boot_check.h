#ifndef BOOT_CHECK_H
#define BOOT_CHECK_H

#include "esp_core_dump.h"
#include "esp_system.h"
#include <gfx_oled_ssd1306.h>
#include "keypad_io.h"

const char* get_exc_cause_name(uint32_t exc_cause);
void show_check_info(GfxOledSSD1306 *display, KeypadIO *keypad);
int boot_check();

#endif