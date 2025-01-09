#include "dpcm.h"

int encode_dpcm(const int8_t *input, int input_size, uint8_t *output) {
    int bit_position = 0;
    int byte_index = 0;
    int previous_value = 0;

    for (int i = 0; i < input_size; ++i) {
        int current_value = input[i];
        int diff = current_value - previous_value;

        int encoded_bit = (diff >= 0) ? 1 : 0;

        if (encoded_bit == 1) {
            output[byte_index] |= (1 << (7 - bit_position));
        }

        bit_position++;
        if (bit_position == 8) {
            bit_position = 0;
            byte_index++;
        }

        previous_value = current_value;
    }

    return (bit_position == 0) ? byte_index : byte_index + 1;
}

int decode_dpcm(const uint8_t *input, int input_size, int8_t *output) {
    int bit_position = 0;
    int byte_index = 0;
    int previous_value = 0;

    for (int i = 0; i < input_size * 8; ++i) {
        int encoded_bit = (input[byte_index] >> (7 - bit_position)) & 1;

        int current_value = previous_value + (encoded_bit == 1 ? 1 : -1);
        output[i] = current_value;

        previous_value = current_value;
        bit_position++;
        if (bit_position == 8) {
            bit_position = 0;
            byte_index++;
        }
    }

    return input_size * 8;
}