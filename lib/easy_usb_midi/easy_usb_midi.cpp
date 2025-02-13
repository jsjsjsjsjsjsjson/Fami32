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

void EASY_USB_MIDI::endAllNote(uint8_t channel) {
    midi_event_packed_t e = {0xB, (midi_code_index_number_t)0, channel, (midi_code_index_number_t)0xB, 123, 0x00};
    MIDI.writePacket((midiEventPacket_t*)&e);
    // printf("MIDI OUT: %X%X %X%X %02X %02X\n", e.cn, e.cin, e.ch, e.event, e.note, e.vol);
}

void EASY_USB_MIDI::cutAllNote(uint8_t channel) {
    midi_event_packed_t e = {0xB, (midi_code_index_number_t)0, channel, (midi_code_index_number_t)0xB, 123, 0x00};
    MIDI.writePacket((midiEventPacket_t*)&e);
    // printf("MIDI OUT: %X%X %X%X %02X %02X\n", e.cn, e.cin, e.ch, e.event, e.note, e.vol);
}

void EASY_USB_MIDI::onMidiEvent(void (*callback)(midi_event_packed_t)) {
    _midiCallback = callback;
}