#include "note2freq.h"

int BASE_FREQ_HZ = 440;

inline float note2period(uint8_t midi_note) {
    return (FCPU_HZ / (16.0f * (BASE_FREQ_HZ * exp2f((midi_note - 69) / 12.0f)))) - 1.0f;
}

inline float period2note(float period) {
    float ratio = FCPU_HZ / (16.0f * BASE_FREQ_HZ * (period + 1.0f));
    float midi_note = 69.0f + 12.0f * log2f(ratio);
    return midi_note + 0.5f;
}

inline uint8_t note2noise(uint8_t midi_note) {
    return (midi_note - 8) & 15;
}

inline float period2freq(float period) {
    return FCPU_HZ / (16.0f * (period + 1.0f));
}

inline float freq2period(float freq) {
    return (FCPU_HZ / (16.0f * freq)) - 1.0f;
}

inline uint8_t item2note(uint8_t note, uint8_t octv) {
    return (note - 1) + ((octv + 2) * 12);
}
