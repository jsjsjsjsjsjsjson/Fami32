#ifndef BASIC_DSP_H
#define BASIC_DSP_H

#include <stdio.h>
#include <math.h>

class LowPassFilter {
public:
    LowPassFilter()
        : lastOutputQ0(0),
          cutoffFreqIn(0),
          sampleRate(44100),
          alpha(1.0f),
          aQ15(0),
          bQ15(32768)
    {}

    int32_t process(int32_t sample) {
        int64_t acc = (int64_t)aQ15 * (int64_t)lastOutputQ0
                    + (int64_t)bQ15 * (int64_t)sample;

        int32_t y = (int32_t)((acc + (1 << 14)) >> 15);
        lastOutputQ0 = y;
        return y;
    }

    void setCutoffFrequency(uint16_t cutoffFreqIn, uint32_t sampleRateIn) {
        sampleRate = sampleRateIn;
        this->cutoffFreqIn = cutoffFreqIn;

        if (sampleRate == 0) {
            alpha = 0.0f;
            aQ15 = 0;
            bQ15 = 32768;
            return;
        }

        if (cutoffFreqIn == 0) {
            alpha = 0.0f;
            aQ15 = 0;
            bQ15 = 32768;
            return;
        }

        uint32_t nyquist = sampleRate / 2;
        uint32_t fc = cutoffFreqIn;
        if (fc >= nyquist) {
            alpha = 0.0f;
            aQ15 = 0;
            bQ15 = 32768;
            return;
        }

        const float x = 2.0f * M_PI * (float)fc / (float)sampleRate;
        float a = expf(-x);

        if (a < 0.0f) a = 0.0f;
        if (a > 0.9999695f) a = 0.9999695f;

        alpha = a;
        int32_t aq = (int32_t)(a * 32768.0f + 0.5f);
        if (aq < 0) aq = 0;
        if (aq > 32767) aq = 32767;

        aQ15 = (uint16_t)aq;
        bQ15 = (uint16_t)(32768 - aq);
    }

private:
    int32_t lastOutputQ0;
    uint16_t cutoffFreqIn;
    uint32_t sampleRate;
    float alpha;

    uint16_t aQ15;
    uint16_t bQ15;
};


class HighPassFilter {
public:
    HighPassFilter()
        : lastInput(0),
          lastOutput(0),
          cutoffFreqIn(0),
          sampleRate(44100),
          alpha(0.0f),
          aQ15(0)
    {}

    int16_t process(int16_t sample) {
        int32_t v = (int32_t)lastOutput + (int32_t)sample - (int32_t)lastInput;
        int32_t y = (int32_t)(((int64_t)aQ15 * (int64_t)v + (1 << 14)) >> 15);

        if (y > 32767) y = 32767;
        else if (y < -32768) y = -32768;

        lastInput = sample;
        lastOutput = (int16_t)y;
        return (int16_t)y;
    }

    void setCutoffFrequency(uint16_t cutoffFreqIn, uint32_t sampleRateIn) {
        sampleRate = sampleRateIn;
        this->cutoffFreqIn = cutoffFreqIn;

        if (sampleRate == 0) {
            alpha = 0.0f;
            aQ15 = 0;
            return;
        }

        if (cutoffFreqIn == 0) {
            alpha = 0.9999695f;
            aQ15 = 32767;
            return;
        }

        uint32_t nyquist = sampleRate / 2;
        uint32_t fc = cutoffFreqIn;
        if (fc >= nyquist) {
            alpha = 0.0f;
            aQ15 = 0;
            return;
        }

        const float x = 2.0f * M_PI * (float)fc / (float)sampleRate;
        float a = expf(-x);

        if (a < 0.0f) a = 0.0f;
        if (a > 0.9999695f) a = 0.9999695f;

        alpha = a;
        int32_t aq = (int32_t)(a * 32768.0f + 0.5f);
        if (aq < 0) aq = 0;
        if (aq > 32767) aq = 32767;
        aQ15 = (uint16_t)aq;
    }

private:
    int16_t lastInput;
    int16_t lastOutput;
    uint16_t cutoffFreqIn;
    uint32_t sampleRate;
    float alpha;
    uint16_t aQ15;
};

#endif
