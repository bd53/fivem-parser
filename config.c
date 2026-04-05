#include "resource.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

Config g_config;

static void get_ini_path(char *out, int size) {
        GetModuleFileNameA(NULL, out, size);
        out[size - 1] = '\0';
        char *last = strrchr(out, '\\');
        if (last) {
                *(last + 1) = '\0';
        } else {
                out[0] = '\0';
        }
        int used = (int)strlen(out);
        int remaining = size - used - 1;
        if (remaining > 0)
                strncat(out, "config.ini", (size_t)remaining);
}

void config_load(void) {
        char ini[MAX_PATH];
        get_ini_path(ini, MAX_PATH);
        g_config.remove_timestamps = GetPrivateProfileIntA("General", "RemoveTimestamps", 0, ini);
        g_config.backup_enabled = GetPrivateProfileIntA("Backup", "Enabled", 0, ini);
        GetPrivateProfileStringA("Backup", "Path", "", g_config.backup_path, MAX_PATH, ini);
        g_config.interval_enabled = GetPrivateProfileIntA("Backup", "IntervalEnabled", 0, ini);
        g_config.interval_minutes = GetPrivateProfileIntA("Backup", "IntervalMinutes", 10, ini);
        if (g_config.interval_minutes < 1)
                g_config.interval_minutes = 1;
        if (g_config.interval_minutes > 60)
                g_config.interval_minutes = 60;
}

void config_save(void) {
        char ini[MAX_PATH];
        get_ini_path(ini, MAX_PATH);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", g_config.remove_timestamps);
        WritePrivateProfileStringA("General", "RemoveTimestamps", buf, ini);
        snprintf(buf, sizeof(buf), "%d", g_config.backup_enabled);
        WritePrivateProfileStringA("Backup", "Enabled", buf, ini);
        WritePrivateProfileStringA("Backup", "Path", g_config.backup_path, ini);
        snprintf(buf, sizeof(buf), "%d", g_config.interval_enabled);
        WritePrivateProfileStringA("Backup", "IntervalEnabled", buf, ini);
        snprintf(buf, sizeof(buf), "%d", g_config.interval_minutes);
        WritePrivateProfileStringA("Backup", "IntervalMinutes", buf, ini);
}
