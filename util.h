#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

void strip_color_codes(char *s);
const char *stristr(const char *h, const char *n);

#ifdef __cplusplus
}
#endif
