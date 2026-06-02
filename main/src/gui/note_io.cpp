#include "note_io.h"

typedef struct {
    bool active;
    uint8_t midi_note;
    uint8_t source;
    uint8_t source_id;
} note_io_assignment_t;

static note_io_assignment_t note_io_assignments[FAMI32_MAX_CHANNELS] = {};

static bool note_io_is_channel_assigned(uint8_t channel) {
    if (channel >= FAMI32_MAX_CHANNELS) {
        return false;
    }
    return note_io_assignments[channel].active;
}

static int8_t note_io_find_assignment(uint8_t midi_note, uint8_t source, uint8_t source_id) {
    const uint32_t channel_count = player.get_channel_count();
    for (uint32_t c = 0; c < channel_count && c < FAMI32_MAX_CHANNELS; c++) {
        if (note_io_assignments[c].active &&
            note_io_assignments[c].midi_note == midi_note &&
            note_io_assignments[c].source == source &&
            note_io_assignments[c].source_id == source_id) {
            return c;
        }
    }
    return -1;
}

int8_t note_io_preview_note_on(uint8_t midi_note, uint8_t volume, uint8_t source, uint8_t source_id) {
    const int8_t existing = note_io_find_assignment(midi_note, source, source_id);
    if (existing >= 0) {
        return existing;
    }

    const uint32_t channel_count = player.get_channel_count();
    const int selected_channel = channel_sel_pos;
    if (selected_channel < 0 || (uint32_t)selected_channel >= channel_count) {
        return -1;
    }

    for (uint32_t c = (uint32_t)selected_channel; c < channel_count && c < FAMI32_MAX_CHANNELS; c++) {
        if (note_io_is_channel_assigned(c)) {
            continue;
        }

        player.channel[c].set_inst(inst_sel_pos);
        player.channel[c].set_note(midi_note);
        if (volume <= 15) {
            player.channel[c].set_vol(volume);
        }
        player.channel[c].note_start();

        note_io_assignments[c].active = true;
        note_io_assignments[c].midi_note = midi_note;
        note_io_assignments[c].source = source;
        note_io_assignments[c].source_id = source_id;
        return c;
    }

    return -1;
}

void note_io_preview_note_off(uint8_t midi_note, uint8_t source, uint8_t source_id) {
    const int8_t channel = note_io_find_assignment(midi_note, source, source_id);
    if (channel < 0) {
        return;
    }

    player.channel[channel].note_end();
    note_io_assignments[channel].active = false;
}

void note_io_preview_all_notes_off() {
    const uint32_t channel_count = player.get_channel_count();
    for (uint32_t c = 0; c < channel_count && c < FAMI32_MAX_CHANNELS; c++) {
        if (!note_io_assignments[c].active) {
            continue;
        }
        player.channel[c].note_cut();
        note_io_assignments[c].active = false;
    }
}

note_io_result_t process_note_io_event(const note_io_event_t &event) {
    note_io_result_t result;
    result.has_note = false;
    result.note = NO_NOTE;
    result.octave = NO_OCT;

    if (event.action == NOTE_IO_ACTION_PRESS) {
        if (event.key > 13) {
            result.has_note = true;
            result.note = event.key - 1;
            result.octave = 0;
            return result;
        }

        const uint8_t note = (event.key % 12) + 1;
        const uint8_t octave = g_octv + (event.key / 12);
        const uint8_t midi_note = item2note(note, octave);

        result.has_note = true;
        result.note = note;
        result.octave = octave;

        if (touch_note) {
            if (edit_mode) {
                player.channel[channel_sel_pos].set_inst(inst_sel_pos);
                player.channel[channel_sel_pos].set_note(midi_note);
                player.channel[channel_sel_pos].note_start();
            } else {
                note_io_preview_note_on(midi_note, NO_VOL, NOTE_IO_SOURCE_TOUCH, event.key);
            }
        }

        if (_midi_output) {
            int8_t midi_channel = channel_sel_pos;
            if (!edit_mode) {
                midi_channel = note_io_find_assignment(midi_note, NOTE_IO_SOURCE_TOUCH, event.key);
                if (midi_channel < 0) {
                    midi_channel = channel_sel_pos;
                }
            }
            MIDI.noteOn(midi_note, 120, midi_channel);
        }
    } else if (event.action == NOTE_IO_ACTION_RELEASE) {
        if (touch_note) {
            uint8_t midi_note = 0;
            int8_t midi_channel = channel_sel_pos;
            if (event.key <= 13) {
                midi_note = item2note((event.key % 12) + 1, g_octv + (event.key / 12));
                if (!edit_mode) {
                    midi_channel = note_io_find_assignment(midi_note, NOTE_IO_SOURCE_TOUCH, event.key);
                    if (midi_channel < 0) {
                        midi_channel = channel_sel_pos;
                    }
                }
            }
            if (edit_mode) {
                player.channel[channel_sel_pos].note_end();
            } else if (event.key <= 13) {
                note_io_preview_note_off(midi_note, NOTE_IO_SOURCE_TOUCH, event.key);
            }
            if (_midi_output && event.key <= 13) {
                MIDI.noteOff(midi_note, 120, midi_channel);
            }
        }
    }

    return result;
}
