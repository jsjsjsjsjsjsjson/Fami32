#ifndef MPR121_KEYPAD_H
#define MPR121_KEYPAD_H

#include <stdint.h>
#include <stddef.h>
#include "keypad_io.h"

#define BUFFER_SIZE 16

union touchKeypadEvent {
    struct {
        uint8_t KEY;
        uint8_t EVENT;
    } bit;
    uint16_t reg;
};

class MPR121_Keypad {
public:
    MPR121_Keypad(uint8_t address0 = 0x5A, uint8_t address1 = 0x5B);

    bool begin();
    void tick();
    bool available();
    touchKeypadEvent read();

private:
    bool init_i2c();
    bool setup_device(uint8_t address);
    bool read_touch_state(uint8_t address, uint16_t *state);
    bool write_register(uint8_t address, uint8_t reg, uint8_t value);
    bool read_registers(uint8_t address, uint8_t reg, uint8_t *data, size_t len);
    void scan_device(uint8_t device_index, uint8_t map_offset, uint16_t *last_state);
    void add_event(touchKeypadEvent e);

    uint8_t i2cAddress0;
    uint8_t i2cAddress1;
    uint16_t lastTouched0;
    uint16_t lastTouched1;

    touchKeypadEvent buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;

    static constexpr uint8_t TOUCHPAD_MAP[24] = {
        255, 255, 255, 255,
        11, 3, 10, 2, 9, 1, 0, 8,
        15, 7, 14, 6, 5, 13, 4, 12,
        255, 255, 255, 255,
    };
};

#endif
