#ifndef BASIC_DSP_H
#define BASIC_DSP_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <math.h>

#include <vector>

#include "src_config.h"

typedef struct {
    int Length = 0;
    float decayRate = 0.0f;
    float dryMix = 0.0f;
    float wetMix = 0.0f;
} delay_config_t;

class AudioDelay {
public:
    int16_t* delayBuffer = nullptr;
    int32_t bufferLength = 0;

    void initialize(const delay_config_t& config) {
        bufferLength = config.Length * (SAMP_RATE / 1000);
        bufferIndex = 0;
        decayRate = config.decayRate;
        dryMix = config.dryMix;
        wetMix = config.wetMix;
        delayBuffer = (int16_t*)realloc(delayBuffer, bufferLength * sizeof(int16_t));
        printf("REL DELAY SIZE: %ld\n", bufferLength * sizeof(int16_t));
        memset(delayBuffer, 0, bufferLength * sizeof(int16_t));
    }

    void free_delay_buffer() {
        free(delayBuffer);
    }

    int16_t process(int16_t inputSample) {
        if (bufferLength) {
            if (delayBuffer == nullptr) {
                return inputSample;
            }
            int16_t delayedSample = delayBuffer[bufferIndex];
            int16_t outputSample = static_cast<int16_t>(dryMix * inputSample + wetMix * delayedSample);
            delayBuffer[bufferIndex] = static_cast<int16_t>(inputSample + delayedSample * decayRate);
            bufferIndex = (bufferIndex + 1) % bufferLength;
            return outputSample;
        } else {
            return inputSample;
        }
    }

    void resize_len(uint16_t len) {
        if (len != bufferLength) {
            delayBuffer = (int16_t*)realloc(delayBuffer, len * (SAMP_RATE * 0.001f) * sizeof(int16_t));
            bufferLength = len;
            memset(delayBuffer, 0, bufferLength * sizeof(int16_t));
        }
    }

    void set_decay(float decay) {
        decayRate = decay;
    }

    void set_dry(float dry) {
        dryMix = dry;
    }

    void set_wet(float wet) {
        wetMix = wet;
    }

private:
    int32_t bufferIndex = 0;
    float decayRate = 0.0f;
    float dryMix = 0.0f;
    float wetMix = 0.0f;
};

typedef struct {
    int numDelays = 0;
    float decay = 0;
    float mix = 0;
    float roomSize = 0;
    float damping = 0;
} reverb_config_t;

class AudioReverb {
public:
    AudioReverb() : sampleRate(0), numDelays(4), feedback(0.0f), dampingFactor(0.0f), mix(0.0f) {}

    void initialize(int sampleRate, const reverb_config_t& config) {
        this->sampleRate = sampleRate;
        this->numDelays = config.numDelays;
        this->config = config;

        delayLines.clear();
        delayIndices.clear();
        lastOutputs.clear();

        for (int i = 0; i < numDelays; ++i) {
            int delayLength = static_cast<int>(sampleRate * config.roomSize * (0.5f + (i * 0.5f) / numDelays));
            delayLines.push_back(std::vector<int16_t>(delayLength, 0));
            delayIndices.push_back(0);
            lastOutputs.push_back(0.0f);
        }

        feedback = config.decay;
        dampingFactor = config.damping;
        mix = config.mix;
    }

    int16_t process(int16_t inputSample) {
        int32_t reverbSample = 0;
        for (int i = 0; i < numDelays; ++i) {
            auto& delayLine = delayLines[i];
            int& delayIndex = delayIndices[i];
            int16_t delayedSample = delayLine[delayIndex];
            lastOutputs[i] = (delayedSample * (1.0f - dampingFactor)) + (lastOutputs[i] * dampingFactor);
            delayLine[delayIndex] = static_cast<int16_t>((inputSample + lastOutputs[i] * feedback));
            delayIndex = (delayIndex + 1) % delayLine.size();
            reverbSample += lastOutputs[i];
        }
        reverbSample /= numDelays;
        return static_cast<int16_t>(inputSample * (1.0f - mix) + reverbSample * mix);
    }

    void setRoomSize(float roomSize) {
        config.roomSize = roomSize;
        initialize(sampleRate, config);
    }

    void setDecay(float decay) {
        config.decay = decay;
        feedback = decay;
    }

    void setDamping(float damping) {
        config.damping = damping;
        dampingFactor = damping;
    }

    void setMix(float mix) {
        config.mix = mix;
        this->mix = mix;
    }

private:
    uint32_t sampleRate;                     // 采样率
    uint8_t numDelays;                      // 延迟线数量
    reverb_config_t config;             // 配置参数
    std::vector<std::vector<int16_t>> delayLines;  // 多个延迟线缓冲区
    std::vector<int> delayIndices;      // 多个延迟线当前索引
    std::vector<float> lastOutputs;     // 每个延迟线的最后输出，用于低通滤波
    float feedback;                     // 反馈系数
    float dampingFactor;                // 阻尼系数（低通滤波的系数）
    float mix;                          // 干湿信号混合比例
};

class LowPassFilter {
public:
    int32_t process(int32_t sample) {
        lastOutput = alpha * sample + (1.0f - alpha) * lastOutput;
        return lastOutput;
    }

    void setCutoffFrequency(uint16_t cutoffFreqIn, uint32_t sampleRateIn) {
        sampleRate = sampleRateIn;
        this->cutoffFreqIn = cutoffFreqIn;
        if (cutoffFreqIn == sampleRate || cutoffFreqIn <= 0) {
            alpha = 1.0f;
        } else {
            alpha = (float)cutoffFreqIn / (cutoffFreqIn + sampleRate);
        }
    }

private:
    int32_t lastOutput;
    uint16_t cutoffFreqIn;
    uint16_t sampleRate;
    float alpha;
};

class HighPassFilter {
public:
    int16_t process(int16_t sample) {
        // High-pass filter formula: output = alpha * (output_prev + input - input_prev)
        int16_t output = roundf(alpha * (lastOutput + sample - lastInput));
        lastInput = sample;
        lastOutput = output;
        return output;
    }

    void setCutoffFrequency(uint16_t cutoffFreqIn, uint32_t sampleRateIn) {
        sampleRate = sampleRateIn;
        this->cutoffFreqIn = cutoffFreqIn;
        if (cutoffFreqIn == sampleRate || cutoffFreqIn <= 0) {
            alpha = 0.0f; // When cutoff frequency is not valid, filter does nothing (bypass).
        } else {
            alpha = (float)sampleRate / (cutoffFreqIn + sampleRate);
        }
    }

private:
    int16_t lastInput;
    int16_t lastOutput;
    uint16_t cutoffFreqIn;
    uint16_t sampleRate;
    float alpha;
};

#endif