#include "note_io.h"

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
            player.channel[channel_sel_pos].set_inst(inst_sel_pos);
            player.channel[channel_sel_pos].set_note(midi_note);
            player.channel[channel_sel_pos].note_start();
        }

        if (_midi_output) {
            MIDI.noteOn(midi_note, 120, channel_sel_pos);
        }
    } else if (event.action == NOTE_IO_ACTION_RELEASE) {
        if (touch_note) {
            player.channel[channel_sel_pos].note_end();
            if (_midi_output && event.key <= 13) {
                const uint8_t midi_note = item2note((event.key % 12) + 1, g_octv + (event.key / 12));
                MIDI.noteOff(midi_note, 120, channel_sel_pos);
            }
        }
    }

    return result;
}
