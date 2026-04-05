#include "resource.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>

static int g_was_running = 0;

static int is_fivem_running(void) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
                return 0;
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        int found = 0;
        if (Process32First(snap, &pe)) {
                do {
                        if (_stricmp(pe.szExeFile, "FiveM_GTAProcess.exe") == 0) {
                                found = 1;
                                break;
                        }
                } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
        return found;
}

static void ensure_directory(const char *path) {
        char tmp[MAX_PATH];
        strncpy(tmp, path, MAX_PATH - 1);
        tmp[MAX_PATH - 1] = '\0';
        for (char *p = tmp + 3; *p; p++) {
                if (*p == '\\' || *p == '/') {
                        char saved = *p;
                        *p = '\0';
                        CreateDirectoryA(tmp, NULL);
                        *p = saved;
                }
        }
        CreateDirectoryA(tmp, NULL);
}

static void do_backup_save(void) {
        if (!g_config.backup_path[0])
                return;
        char logpath[MAX_PATH * 2];
        if (!find_latest_log(logpath, sizeof(logpath)))
                return;
        char *parsed = parse_log_file(logpath, g_config.remove_timestamps);
        if (!parsed || !parsed[0]) {
                free(parsed);
                return;
        }
        static const char *months[] = {
                "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
        };
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (st.wMonth < 1 || st.wMonth > 12) {
                free(parsed);
                return;
        }
        char dir[MAX_PATH + 64];
        snprintf(dir, sizeof(dir), "%s\\%04u\\%s", g_config.backup_path, (unsigned)st.wYear, months[st.wMonth - 1]);
        ensure_directory(dir);
        char filepath[MAX_PATH + 128];
        snprintf(filepath, sizeof(filepath), "%s\\%02u.%s.%04u-%02u.%02u.%02u.txt", dir, (unsigned)st.wDay, months[st.wMonth - 1], (unsigned)st.wYear, (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond);
        FILE *f = fopen(filepath, "wb");
        if (f) {
                fwrite(parsed, 1, strlen(parsed), f);
                fclose(f);
        }
        free(parsed);
}

void backup_on_timer(void) {
        if (!g_config.backup_enabled)
                return;
        int running = is_fivem_running();
        if (g_was_running && !running)
                do_backup_save();
        g_was_running = running;
}

void backup_on_interval(void) {
        if (!g_config.interval_enabled)
                return;
        if (!is_fivem_running())
                return;
        do_backup_save();
}
