#include "dpcm.h"

int encode_dpcm(const uint8_t *input, int input_size, uint8_t *output)
{
    int predictor = 64;
    const int step = 2;
    int acc = 0;

    int total_bytes = (input_size + 7) / 8;

    for (int i = 0; i < total_bytes; i++) {
        uint8_t out_byte = 0;

        for (int bit_index = 0; bit_index < 8; bit_index++) {
            int sample_index = i * 8 + bit_index;
            if (sample_index >= input_size) {
                break;
            }

            int error = (int)input[sample_index] - predictor;
            acc += error;

            if (acc >= 0) {
                out_byte |= (1 << bit_index);
                predictor += step;
                if (predictor > 127) {
                    predictor = 127;
                }
                acc -= step;
            } else {
                predictor -= step;
                if (predictor < 0) {
                    predictor = 0;
                }
                acc += step;
            }
        }

        output[i] = out_byte;
    }

    return total_bytes;
}

int decode_dpcm(const uint8_t *input, int input_size, uint8_t *output)
{
    int level = 64;

    for (int i = 0; i < input_size; i++) {
        uint8_t byte_val = input[i];

        for (int bit_index = 0; bit_index < 8; bit_index++) {
            int bit = (byte_val & 1);
            byte_val >>= 1;

            if (bit) {
                if (level <= 125) {
                    level += 2;
                } else {
                    level = 127;
                }
            } else {
                if (level >= 2) {
                    level -= 2;
                } else {
                    level = 0;
                }
            }

            output[i * 8 + bit_index] = (uint8_t)level;
        }
    }

    return input_size * 8;
}