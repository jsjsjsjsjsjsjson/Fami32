#ifndef CHIPBOX_PIN_H
#define CHIPBOX_PIN_H

#include <stdint.h>

// KEYPAD
#define KEY_UP 3
#define KEY_DOWN 9
#define KEY_L 0
#define KEY_R 6
#define KEY_OK 1
#define KEY_S 4
#define KEY_P 10
#define KEY_MENU 2
#define KEY_NAVI 5
#define KEY_BACK 7
#define KEY_OCTU 11
#define KEY_OCTD 8

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3

const char KEYPAD_MAP[KEYPAD_ROWS][KEYPAD_COLS] = {
    {KEY_L,    KEY_OK,   KEY_MENU},
    {KEY_UP,   KEY_S,    KEY_NAVI},
    {KEY_R,    KEY_BACK, KEY_OCTD},
    {KEY_DOWN, KEY_P,    KEY_OCTU}
};

#define KEYPAD_R0 47
#define KEYPAD_R1 18
#define KEYPAD_R2 45
#define KEYPAD_R3 46

#define KEYPAD_C0 38
#define KEYPAD_C1 39
#define KEYPAD_C2 48
// KEYPAD

// TOUCHPAD
#define TOUCHPAD_SDA 8
#define TOUCHPAD_SCL 9
#define TOUCHPAD0_ADDRS 0x5A
#define TOUCHPAD1_ADDRS 0x5B
const uint8_t TOUCHPAD_MAP[24] =
    {255, 255, 255, 255,
        11, 3, 10, 2, 9, 1, 0, 8, 15, 7, 14, 6, 5, 13, 4, 12,
    255, 255, 255, 255};
// TOUCHPAD

// AUDIO OUTPUT (PCM5102A)
#define PCM5102A_BCK 42
#define PCM5102A_WS 40
#define PCM5102A_DIN 41
// AUTIO OUTPUT (PCM5102A)

// DISPLAY (SSD1309 SPI)
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_SCL 17
#define DISPLAY_SDA 16
#define DISPLAY_DC 7
#define DISPLAY_RESET 15
#define DISPLAY_CS 6
// DISPLAY (SSD1309 SPI)

// LED (WS2812)
#define LED_NUM 16
#define LED_GPIO 11
// LED (WS2812)

#define DBG_PRINTF(fmt, ...) \
    do { \
        if (_debug_print) { \
            printf(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#endif // CHIPBOX_PIN_H
