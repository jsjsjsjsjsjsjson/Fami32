#ifndef BOOT_CHECK_H
#define BOOT_CHECK_H

#include "esp_core_dump.h"
#include "esp_system.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_Keypad.h>

const char* get_exc_cause_name(uint32_t exc_cause);
void show_check_info(Adafruit_SSD1306 *display, Adafruit_Keypad *keypad);
int boot_check();

#endif