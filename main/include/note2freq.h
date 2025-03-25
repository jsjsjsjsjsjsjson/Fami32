#ifndef NOTE2FREQ_H
#define NOTE2FREQ_H

#include <stdint.h>
#include <math.h>

#define FCPU_HZ 1789773.0f
extern int BASE_FREQ_HZ;

float note2period(uint8_t midi_note);
float period2note(float period);
uint8_t note2noise(uint8_t midi_note);
float period2freq(float period);
float freq2period(float freq);
uint8_t item2note(uint8_t note, uint8_t octv);

#endif