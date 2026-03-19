#include "gfx_oled_ssd1306.h"

GfxOledSSD1306::GfxOledSSD1306(int16_t w, int16_t h)
    : Adafruit_GFX(w, h),
      panel_(nullptr),
      buffer_(nullptr),
      buffer_size_(static_cast<size_t>(w) * static_cast<size_t>(h) / 8)
{
    buffer_ = (uint8_t*)calloc(1, buffer_size_);
    flush_buffer_ = (uint8_t*)malloc(buffer_size_);

    assert(buffer_ && flush_buffer_);
}

GfxOledSSD1306::~GfxOledSSD1306() {
    free(buffer_);
    buffer_ = nullptr;
}

esp_err_t GfxOledSSD1306::begin(esp_lcd_panel_handle_t panel)
{
    if (panel == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }
    if (buffer_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    panel_ = panel;
    return ESP_OK;
}

bool GfxOledSSD1306::isReady() const {
    return (panel_ != nullptr) && (buffer_ != nullptr);
}

uint8_t *GfxOledSSD1306::buffer() {
    return buffer_;
}

inline void GfxOledSSD1306::setPixelRaw(int16_t x, int16_t y, bool on) {
    if (x < 0 || x >= width() || y < 0 || y >= height()) return;

    size_t index = x + (y / 8) * WIDTH;
    uint8_t mask = 1 << (y & 7);

    if (on)
        buffer_[index] |= mask;
    else
        buffer_[index] &= ~mask;
}

void GfxOledSSD1306::drawPixel(int16_t x, int16_t y, uint16_t color) {
    setPixelRaw(x, y, color != 0);
}

uint16_t GfxOledSSD1306::getPixel(int16_t x, int16_t y)
{
    if (x < 0 || x >= width() || y < 0 || y >= height()) {
        return 0;
    }

    size_t index = x + (y >> 3) * WIDTH;
    uint8_t mask = 1 << (y & 7);

    return (buffer_[index] & mask) ? OLED_COLOR_WHITE : OLED_COLOR_BLACK;
}

void GfxOledSSD1306::fillScreen(uint16_t color) {
    memset(buffer_, color ? 0xFF : 0x00, buffer_size_);
}

void GfxOledSSD1306::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= HEIGHT) return;

    if (x < 0) {
        w += x;
        x = 0;
    }

    if (x + w > WIDTH) {
        w = WIDTH - x;
    }

    if (w <= 0) return;

    uint8_t *p = &buffer_[(y >> 3) * WIDTH + x];
    uint8_t mask = 1 << (y & 7);

    if (color == OLED_COLOR_WHITE) {
        while (w--) *p++ |= mask;
    } else {
        mask = ~mask;
        while (w--) *p++ &= mask;
    }
}

void GfxOledSSD1306::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (x < 0 || x >= WIDTH) return;

    if (y < 0) {
        h += y;
        y = 0;
    }

    if (y + h > HEIGHT) {
        h = HEIGHT - y;
    }

    if (h <= 0) return;

    uint8_t *p = &buffer_[(y >> 3) * WIDTH + x];
    uint8_t mod = y & 7;

    // --- partial head ---
    if (mod) {
        uint8_t bits = 8 - mod;

        static const uint8_t premask[8] = {
            0x00, 0x80, 0xC0, 0xE0,
            0xF0, 0xF8, 0xFC, 0xFE
        };

        uint8_t mask = premask[bits];

        if (h < bits) {
            mask &= (0xFF >> (bits - h));
        }

        if (color == OLED_COLOR_WHITE) *p |= mask;
        else *p &= ~mask;

        p += WIDTH;
        h -= bits;
    }

    // --- full bytes ---
    if (h >= 8) {
        uint8_t val = (color == OLED_COLOR_WHITE) ? 0xFF : 0x00;

        do {
            *p = val;
            p += WIDTH;
            h -= 8;
        } while (h >= 8);
    }

    // --- tail ---
    if (h > 0) {
        static const uint8_t postmask[8] = {
            0x00, 0x01, 0x03, 0x07,
            0x0F, 0x1F, 0x3F, 0x7F
        };

        uint8_t mask = postmask[h];

        if (color == OLED_COLOR_WHITE) *p |= mask;
        else *p &= ~mask;
    }
}

void GfxOledSSD1306::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;

    // clip
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > WIDTH) {
        w = WIDTH - x;
    }
    if (y + h > HEIGHT) {
        h = HEIGHT - y;
    }

    if (w <= 0 || h <= 0) return;

    uint8_t fill_byte = (color == OLED_COLOR_WHITE) ? 0xFF : 0x00;

    for (int16_t i = x; i < x + w; ++i) {

        uint8_t *p = &buffer_[(y >> 3) * WIDTH + i];
        int16_t remaining = h;
        uint8_t mod = y & 7;

        // --- head ---
        if (mod) {
            uint8_t bits = 8 - mod;

            static const uint8_t premask[8] = {
                0x00, 0x80, 0xC0, 0xE0,
                0xF0, 0xF8, 0xFC, 0xFE
            };

            uint8_t mask = premask[bits];

            if (remaining < bits) {
                mask &= (0xFF >> (bits - remaining));
            }

            if (color == OLED_COLOR_WHITE) *p |= mask;
            else *p &= ~mask;

            p += WIDTH;
            remaining -= bits;
        }

        // --- full bytes ---
        while (remaining >= 8) {
            *p = fill_byte;
            p += WIDTH;
            remaining -= 8;
        }

        // --- tail ---
        if (remaining > 0) {
            static const uint8_t postmask[8] = {
                0x00, 0x01, 0x03, 0x07,
                0x0F, 0x1F, 0x3F, 0x7F
            };

            uint8_t mask = postmask[remaining];

            if (color == OLED_COLOR_WHITE) *p |= mask;
            else *p &= ~mask;
        }
    }
}

void GfxOledSSD1306::clearDisplay() {
    fillScreen(0);
}

esp_err_t GfxOledSSD1306::display() {
    if (!isReady()) return ESP_ERR_INVALID_STATE;
    memcpy(flush_buffer_, buffer_, buffer_size_);
    taskYIELD();
    return esp_lcd_panel_draw_bitmap(panel_, 0, 0, WIDTH, HEIGHT, flush_buffer_);
}