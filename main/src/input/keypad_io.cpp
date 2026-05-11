#include "keypad_io.h"

#include <cstring>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

namespace {
constexpr const char *TAG = "KeypadIO";
constexpr uint32_t SETTLING_DELAY_US = 20;
} // namespace

KeypadIO::KeypadIO(const uint8_t *keymap,
                   const uint8_t *rowPins,
                   const uint8_t *colPins,
                   size_t numRows,
                   size_t numCols)
    : keymap_(keymap),
      rowPins_(rowPins),
      colPins_(colPins),
      numRows_(numRows),
      numCols_(numCols),
      head_(0),
      tail_(0),
      count_(0) {
    memset(keyStates_, 0, sizeof(keyStates_));
}

bool KeypadIO::begin() {
    if (keymap_ == nullptr || rowPins_ == nullptr || colPins_ == nullptr || numRows_ == 0 || numCols_ == 0) {
        ESP_LOGE(TAG, "invalid keypad config");
        return false;
    }

    if ((numRows_ * numCols_) > sizeof(keyStates_)) {
        ESP_LOGE(TAG, "too many keys: %u", static_cast<unsigned>(numRows_ * numCols_));
        return false;
    }

    clear();

    for (size_t i = 0; i < numCols_; ++i) {
        gpio_config_t colCfg = {};
        colCfg.pin_bit_mask = (1ULL << colPins_[i]);
        colCfg.mode = GPIO_MODE_OUTPUT;
        colCfg.pull_up_en = GPIO_PULLUP_DISABLE;
        colCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        colCfg.intr_type = GPIO_INTR_DISABLE;
        esp_err_t err = gpio_config(&colCfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "failed to config col pin %u: %s", static_cast<unsigned>(colPins_[i]), esp_err_to_name(err));
            return false;
        }
        gpio_set_level(static_cast<gpio_num_t>(colPins_[i]), 1);
    }

    for (size_t i = 0; i < numRows_; ++i) {
        gpio_config_t rowCfg = {};
        rowCfg.pin_bit_mask = (1ULL << rowPins_[i]);
        rowCfg.mode = GPIO_MODE_INPUT;
        rowCfg.pull_up_en = GPIO_PULLUP_ENABLE;
        rowCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        rowCfg.intr_type = GPIO_INTR_DISABLE;
        esp_err_t err = gpio_config(&rowCfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "failed to config row pin %u: %s", static_cast<unsigned>(rowPins_[i]), esp_err_to_name(err));
            return false;
        }
    }

    return true;
}

void KeypadIO::tick() {
    for (size_t c = 0; c < numCols_; ++c) {
        gpio_set_level(static_cast<gpio_num_t>(colPins_[c]), 0);
        esp_rom_delay_us(SETTLING_DELAY_US);

        for (size_t r = 0; r < numRows_; ++r) {
            const size_t index = r * numCols_ + c;
            const bool pressed = gpio_get_level(static_cast<gpio_num_t>(rowPins_[r])) == 0;
            uint8_t currentState = keyStates_[index];

            currentState &= static_cast<uint8_t>(~(STATE_JUST_PRESSED | STATE_JUST_RELEASED));
            if (pressed && !(currentState & STATE_PRESSED)) {
                currentState |= static_cast<uint8_t>(STATE_PRESSED | STATE_JUST_PRESSED);
                keypadEvent event = {};
                event.bit.EVENT = KEY_JUST_PRESSED;
                event.bit.KEY = keymap_[index];
                event.bit.ROW = r;
                event.bit.COL = c;
                pushEvent(event);
            } else if (!pressed && (currentState & STATE_PRESSED)) {
                currentState &= static_cast<uint8_t>(~STATE_PRESSED);
                currentState |= STATE_JUST_RELEASED;
                keypadEvent event = {};
                event.bit.EVENT = KEY_JUST_RELEASED;
                event.bit.KEY = keymap_[index];
                event.bit.ROW = r;
                event.bit.COL = c;
                pushEvent(event);
            }

            keyStates_[index] = currentState;
        }

        gpio_set_level(static_cast<gpio_num_t>(colPins_[c]), 1);
    }
}

bool KeypadIO::justPressed(uint8_t key, bool clear) {
    int index = findKeyIndex(key);
    if (index < 0) {
        return false;
    }

    bool pressed = (keyStates_[index] & STATE_JUST_PRESSED) != 0;
    if (clear) {
        keyStates_[index] &= static_cast<uint8_t>(~STATE_JUST_PRESSED);
    }
    return pressed;
}

bool KeypadIO::justReleased(uint8_t key) {
    int index = findKeyIndex(key);
    if (index < 0) {
        return false;
    }

    bool released = (keyStates_[index] & STATE_JUST_RELEASED) != 0;
    keyStates_[index] &= static_cast<uint8_t>(~STATE_JUST_RELEASED);
    return released;
}

bool KeypadIO::isPressed(uint8_t key) const {
    int index = findKeyIndex(key);
    if (index < 0) {
        return false;
    }
    return (keyStates_[index] & STATE_PRESSED) != 0;
}

bool KeypadIO::isReleased(uint8_t key) const {
    return !isPressed(key);
}

int KeypadIO::available() const {
    return static_cast<int>(count_);
}

keypadEvent KeypadIO::read() {
    keypadEvent event = {};
    if (count_ == 0) {
        return event;
    }

    event = buffer_[tail_];
    tail_ = (tail_ + 1) % BUFFER_SIZE;
    --count_;
    return event;
}

void KeypadIO::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
    memset(keyStates_, 0, sizeof(keyStates_));
}

void KeypadIO::pushEvent(const keypadEvent &event) {
    if (count_ >= BUFFER_SIZE) {
        tail_ = (tail_ + 1) % BUFFER_SIZE;
        --count_;
    }

    buffer_[head_] = event;
    head_ = (head_ + 1) % BUFFER_SIZE;
    ++count_;
}

int KeypadIO::findKeyIndex(uint8_t key) const {
    for (size_t i = 0; i < numRows_ * numCols_; ++i) {
        if (keymap_[i] == key) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
