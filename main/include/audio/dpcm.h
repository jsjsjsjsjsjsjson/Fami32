#ifndef DPCM_H
#define DPCM_H

#include <stdint.h>

int encode_dpcm(const uint8_t *input, int input_size, uint8_t *output);
int decode_dpcm(const uint8_t *input, int input_size, uint8_t *output);

#endif