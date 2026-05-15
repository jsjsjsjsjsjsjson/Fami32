#ifndef VGM_EXPORTER_H
#define VGM_EXPORTER_H

#include <stdint.h>

#include "ftm_file.h"

typedef enum {
    VGM_EXPORT_OK = 0,
    VGM_EXPORT_FILE_ERROR = -1,
    VGM_EXPORT_DPCM_TOO_LARGE = -2,
    VGM_EXPORT_RENDER_LIMIT = -3,
} vgm_export_result_t;

typedef bool (*vgm_export_progress_cb)(uint32_t ticks, uint32_t samples, void *user);

vgm_export_result_t export_nes_vgm(FTM_FILE *ftm_data,
                                   const char *filename,
                                   vgm_export_progress_cb progress_cb = nullptr,
                                   void *progress_user = nullptr);

const char *vgm_export_result_str(vgm_export_result_t result);

#endif
