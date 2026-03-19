#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
}

#include "Adafruit_GFX.h"

#define OLED_COLOR_BLACK   0
#define OLED_COLOR_WHITE   1
#define OLED_COLOR_INVERT  2

class GfxOledSSD1306 : public Adafruit_GFX {
public:
    GfxOledSSD1306(int16_t w, int16_t h);
    ~GfxOledSSD1306();

    GfxOledSSD1306(const GfxOledSSD1306&) = delete;
    GfxOledSSD1306& operator=(const GfxOledSSD1306&) = delete;

    esp_err_t begin(esp_lcd_panel_handle_t panel);
    bool isReady() const;

    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    uint16_t getPixel(int16_t x, int16_t y);
    void fillScreen(uint16_t color) override;
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

    void clearDisplay();
    esp_err_t display();

    uint8_t *buffer();

private:
    inline void setPixelRaw(int16_t x, int16_t y, bool on);

private:
    esp_lcd_panel_handle_t panel_;
    uint8_t *buffer_;
    uint8_t *flush_buffer_; 
    size_t buffer_size_;
};