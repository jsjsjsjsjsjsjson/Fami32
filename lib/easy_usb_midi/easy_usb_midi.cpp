#include "easy_usb_midi.h"

EASY_USB_MIDI::EASY_USB_MIDI() : _midiQueue(NULL), _midiCallback(NULL) {}

EASY_USB_MIDI::~EASY_USB_MIDI() {
    if (_midiQueue != NULL) {
        vQueueDelete(_midiQueue);
    }
}

void EASY_USB_MIDI::begin() {
    _midiQueue = xQueueCreate(16, sizeof(midi_event_packed_t));
    if (_midiQueue == NULL) {
        printf("Failed to create MIDI queue\n");
        return;
    }
    USB.begin();
}

void EASY_USB_MIDI::tick() {
    midi_event_packed_t midi_event;
    if (MIDI.readPacket((midiEventPacket_t*)&midi_event)) {
        if (xQueueSend(_midiQueue, &midi_event, 0) != pdPASS) {
            printf("Failed to send MIDI event to queue\n");
        }
        if (_midiCallback != NULL) {
            _midiCallback(midi_event);
        }
    }
}

int EASY_USB_MIDI::available() {
    return uxQueueMessagesWaiting(_midiQueue);
}

midi_event_packed_t EASY_USB_MIDI::read() {
    midi_event_packed_t event;
    if (xQueueReceive(_midiQueue, &event, 0) == pdTRUE) {
        return event;
    }
    return {0};
}

void EASY_USB_MIDI::clear() {
    xQueueReset(_midiQueue);
}

void EASY_USB_MIDI::onMidiEvent(void (*callback)(midi_event_packed_t)) {
    _midiCallback = callback;
}