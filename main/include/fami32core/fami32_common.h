#ifndef FAMI32_COMMON_H
#define FAMI32_COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <vector>

#include "ftm_file.h"
#include "basic_dsp.h"
#ifndef DESKTOP_BUILD
#include <USBMIDI.h>
#endif

extern "C" {
#include "src_config.h"
#include "note2freq.h"
#include "wave_table.h"
}

extern bool _debug_print;
extern bool _midi_output;

#ifndef DESKTOP_BUILD
extern USBMIDI MIDI;
#endif

#define VOL_SEQU 0
#define ARP_SEQU 1
#define PIT_SEQU 2
#define HPI_SEQU 3
#define DTY_SEQU 4

typedef enum {
    SEQU_STOP,
    SEQU_PLAYING,
    SEQU_RELEASE
} note_stat_t;

extern int g_vol;

#endif