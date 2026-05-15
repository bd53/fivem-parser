#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        int show_timestamps;
        int wrap_width;
        int png_bg_r, png_bg_g, png_bg_b, png_bg_a;
} Config;

extern Config g_config;

void config_load(void);
void config_save(void);

#ifdef __cplusplus
}
#endif
