#include "USBMIDI.h"

// Log tag
static const char *TAG = "USBMIDI";

// Constructor
USBMIDI::USBMIDI() : initialized(false), readTaskHandle(NULL), callback(NULL) {}

// Destructor
USBMIDI::~USBMIDI() {
    if (readTaskHandle) {
        vTaskDelete(readTaskHandle);
    }
}

// Initialize USB MIDI
bool USBMIDI::begin() {
    if (initialized) {
        ESP_LOGE(TAG, "USBMIDI already initialized");
        return false;
    }

    // Create a task for reading MIDI data
    xTaskCreatePinnedToCore(midiReadTask, "midi_read_task", 4096, this, 6, &readTaskHandle, 0);

    initialized = true;
    ESP_LOGI(TAG, "USBMIDI initialized");
    return true;
}

// Set MIDI callback function
void USBMIDI::setCallback(midi_callback_t callback) {
    this->callback = callback;
}

// Send Note On message
void USBMIDI::noteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    uint8_t data[3] = {(uint8_t)(NOTE_ON | channel), note, velocity};
    tud_midi_stream_write(0, data, 3);
    // ESP_LOGI(TAG, "Note On: Channel %d, Note %d, Velocity %d", channel + 1, note, velocity);
}

// Send Note Off message
void USBMIDI::noteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    uint8_t data[3] = {(uint8_t)(NOTE_OFF | channel), note, velocity};
    tud_midi_stream_write(0, data, 3);
    // ESP_LOGI(TAG, "Note Off: Channel %d, Note %d, Velocity %d", channel + 1, note, velocity);
}

// Send Control Change message
void USBMIDI::controlChange(uint8_t control, uint8_t value, uint8_t channel) {
    uint8_t data[3] = {(uint8_t)(0xB0 | channel), control, value};
    tud_midi_stream_write(0, data, 3);
    // ESP_LOGI(TAG, "Control Change: Channel %d, Control %d, Value %d", channel + 1, control, value);
}

// Send Program Change message
void USBMIDI::programChange(uint8_t program, uint8_t channel) {
    uint8_t data[2] = {(uint8_t)(0xC0 | channel), program};
    tud_midi_stream_write(0, data, 2);
    // ESP_LOGI(TAG, "Program Change: Channel %d, Program %d", channel + 1, program);
}

// Read MIDI packet
bool USBMIDI::readPacket(midiEventPacket_t *packet) {
    return tud_midi_packet_read((uint8_t *)packet);
}

// Write MIDI packet
bool USBMIDI::writePacket(midiEventPacket_t *packet) {
    if (!isConnected()) {
        ESP_LOGE(TAG, "USB MIDI not connected");
        return false;
    }

    // Send the MIDI packet
    bool result = tud_midi_packet_write((uint8_t *)packet);
    if (result) {
        // ESP_LOGI(TAG, "MIDI Packet Sent: %02X %02X %02X %02X", packet->header, packet->byte1, packet->byte2, packet->byte3);
    } else {
        ESP_LOGE(TAG, "Failed to send MIDI packet");
    }
    return result;
}

// Check if USB MIDI is connected
bool USBMIDI::isConnected() {
    return tud_midi_mounted();
}

// Task for reading MIDI data
void USBMIDI::midiReadTask(void *arg) {
    USBMIDI *midi = (USBMIDI *)arg;
    midiEventPacket_t packet;

    for (;;) {
        while (midi->readPacket(&packet)) {
            ESP_LOGI(TAG, "MIDI Packet Received: %02X %02X %02X %02X", packet.header, packet.byte1, packet.byte2, packet.byte3);
            if (midi->callback) {
                midi->callback(packet);
            }
        }
        vTaskDelay(1);
    }
}