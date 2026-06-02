#ifndef NOTE_IO_H
#define NOTE_IO_H

#include "gui_common.h"

typedef enum {
    NOTE_IO_ACTION_NONE = 0,
    NOTE_IO_ACTION_PRESS,
    NOTE_IO_ACTION_RELEASE,
} note_io_action_t;

typedef struct {
    uint8_t key;
    note_io_action_t action;
} note_io_event_t;

typedef struct {
    bool has_note;
    uint8_t note;
    uint8_t octave;
} note_io_result_t;

typedef enum {
    NOTE_IO_SOURCE_TOUCH = 0,
    NOTE_IO_SOURCE_MIDI,
} note_io_source_t;

note_io_result_t process_note_io_event(const note_io_event_t &event);
int8_t note_io_preview_note_on(uint8_t midi_note, uint8_t volume, uint8_t source, uint8_t source_id);
void note_io_preview_note_off(uint8_t midi_note, uint8_t source, uint8_t source_id);
void note_io_preview_all_notes_off();

#endif // NOTE_IO_H
