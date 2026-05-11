#ifndef KEYPAD_IO_H
#define KEYPAD_IO_H

#include <stdint.h>
#include <stddef.h>

#define KEY_JUST_RELEASED (0)
#define KEY_JUST_PRESSED (1)

union keypadEvent {
    struct {
        uint8_t KEY;
        uint8_t EVENT;
        uint8_t ROW;
        uint8_t COL;
    } bit;
    uint32_t reg;
};

class KeypadIO {
public:
    KeypadIO(const uint8_t *keymap,
             const uint8_t *rowPins,
             const uint8_t *colPins,
             size_t numRows,
             size_t numCols);

    bool begin();
    void tick();

    bool justPressed(uint8_t key, bool clear = true);
    bool justReleased(uint8_t key);
    bool isPressed(uint8_t key) const;
    bool isReleased(uint8_t key) const;

    int available() const;
    keypadEvent read();
    void clear();

private:
    void pushEvent(const keypadEvent &event);
    int findKeyIndex(uint8_t key) const;

    const uint8_t *keymap_;
    const uint8_t *rowPins_;
    const uint8_t *colPins_;
    size_t numRows_;
    size_t numCols_;

    static constexpr size_t BUFFER_SIZE = 32;
    keypadEvent buffer_[BUFFER_SIZE];
    size_t head_;
    size_t tail_;
    size_t count_;

    static constexpr uint8_t STATE_PRESSED = (1U << 0);
    static constexpr uint8_t STATE_JUST_PRESSED = (1U << 1);
    static constexpr uint8_t STATE_JUST_RELEASED = (1U << 2);
    uint8_t keyStates_[32];
};

#endif // KEYPAD_IO_H
