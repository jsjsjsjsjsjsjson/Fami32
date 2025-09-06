#ifndef _USBMIDI_H_INCLUDED
#define _USBMIDI_H_INCLUDED

#include "esp_err.h"
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// MIDI message types
#define NOTE_OFF 0x80
#define NOTE_ON  0x90

// MIDI packet structure
typedef struct {
    uint8_t header;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
} midiEventPacket_t;

typedef struct PACKED_ATTR {
    uint8_t cn : 4;
    midi_code_index_number_t cin : 4;
    uint8_t ch : 4;
    midi_code_index_number_t event : 4;
    uint8_t note;
    uint8_t vol;
} midiEventData_t;

// Callback function type
typedef void (*midi_callback_t)(midiEventPacket_t);

class USBMIDI {
public:
    USBMIDI();
    ~USBMIDI();

    // Initialize USB MIDI
    bool begin();

    // Set MIDI callback function
    void setCallback(midi_callback_t callback);

    // Send MIDI messages
    void noteOn(uint8_t note, uint8_t velocity = 127, uint8_t channel = 0);
    void noteOff(uint8_t note, uint8_t velocity = 0, uint8_t channel = 0);
    void controlChange(uint8_t control, uint8_t value, uint8_t channel = 0);
    void programChange(uint8_t program, uint8_t channel = 0);

    // Read MIDI messages
    bool readPacket(midiEventPacket_t *packet);

    // Write MIDI packet
    bool writePacket(midiEventPacket_t *packet);

    // Check if USB MIDI is connected
    bool isConnected();

private:
    // TinyUSB descriptors
    static const char *s_str_desc[5];
    static const uint8_t s_midi_cfg_desc[];

    // Task for reading MIDI data
    static void midiReadTask(void *arg);

    // Internal state
    bool initialized;
    TaskHandle_t readTaskHandle;
    midi_callback_t callback;
};

#endif /* _USBMIDI_H_INCLUDED */