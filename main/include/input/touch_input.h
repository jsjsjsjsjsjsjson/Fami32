#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t key;
    uint8_t event;
} touch_input_event_t;

void touch_input_init();
bool touch_input_push_event(uint8_t key, uint8_t event);
bool touch_input_pop_event(touch_input_event_t *event);
void touch_input_flush();

#endif // TOUCH_INPUT_H
