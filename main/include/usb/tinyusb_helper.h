#ifndef TINYUSB_HELPER_H
#define TINYUSB_HELPER_H

#include "tinyusb.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

bool init_tinyusb(const uint8_t *cfg_desc, size_t desc_len);

#endif