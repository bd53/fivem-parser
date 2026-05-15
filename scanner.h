#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        char raw[8192];
        char plain[8192];
} ScannedMsg;

int scanner_start(void);
void scanner_stop(void);
int scanner_is_running(void);
int scanner_poll(ScannedMsg *out, int max_count);

#ifdef __cplusplus
}
#endif
