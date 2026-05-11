#include "touch_input.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t s_touch_event_queue = NULL;
static constexpr uint32_t TOUCH_EVENT_QUEUE_LEN = 32;

void touch_input_init() {
    if (s_touch_event_queue == NULL) {
        s_touch_event_queue = xQueueCreate(TOUCH_EVENT_QUEUE_LEN, sizeof(touch_input_event_t));
    }
}

bool touch_input_push_event(uint8_t key, uint8_t event) {
    if (s_touch_event_queue == NULL) return false;
    touch_input_event_t touch_event;
    touch_event.key = key;
    touch_event.event = event;
    return xQueueSendToBack(s_touch_event_queue, &touch_event, 0) == pdTRUE;
}

bool touch_input_pop_event(touch_input_event_t *event) {
    if (event == NULL || s_touch_event_queue == NULL) return false;
    return xQueueReceive(s_touch_event_queue, event, 0) == pdTRUE;
}

void touch_input_flush() {
    if (s_touch_event_queue != NULL) {
        xQueueReset(s_touch_event_queue);
    }
}
