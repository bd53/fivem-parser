#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        int remove_timestamps;
        int backup_enabled;
        char backup_path[MAX_PATH];
        int interval_enabled;
        int interval_minutes;
} Config;

extern Config g_config;

void config_load(void);
void config_save(void);

void strip_color_codes(char *s);
int rtrim_newline(char *s);
char *parse_log_file(const char *path, int remove_timestamps);
int find_latest_log(char *out, int out_size);
void get_fivem_logs_dir(char *out, int out_size);

typedef struct {
        char timestamp[64];
        char raw[8192];
        char plain[8192];
} ChatEntry;

typedef struct {
        ChatEntry *entries;
        int count;
        int capacity;
} ChatLog;

ChatLog *parse_log_chat(const char *path, int remove_timestamps);
void chatlog_free(ChatLog *log);

void backup_on_timer(void);
void backup_on_interval(void);

#ifdef __cplusplus
}
#endif
