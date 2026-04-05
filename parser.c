#include "resource.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void strip_color_codes(char *s) {
        char buf[8192];
        int j = 0;
        for (int i = 0; s[i] && j < (int)sizeof(buf) - 1; i++) {
                if (s[i] == '^') {
                        char nx = s[i + 1];
                        if (nx == '#') {
                                int k = i + 2;
                                while (s[k] && ((s[k] >= '0' && s[k] <= '9') || (s[k] >= 'a' && s[k] <= 'f') || (s[k] >= 'A' && s[k] <= 'F')))
                                        k++;
                                i = k - 1;
                                continue;
                        }
                        if ((nx >= '0' && nx <= '9') || nx == '*' || nx == '_' || nx == 'r' || nx == '~') {
                                i++;
                                continue;
                        }
                }
                if (s[i] == '~') {
                        int k = i + 1;
                        while (s[k] && s[k] != '~')
                                k++;
                        if (s[k] == '~') {
                                i = k;
                                continue;
                        }
                }
                buf[j++] = s[i];
        }
        buf[j] = '\0';
        memcpy(s, buf, (size_t)j + 1);
}

int rtrim_newline(char *s) {
        int n = (int)strlen(s);
        while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r'))
                s[--n] = '\0';
        return n;
}

void get_fivem_logs_dir(char *out, int out_size) {
        char local[MAX_PATH];
        if (!GetEnvironmentVariableA("LOCALAPPDATA", local, MAX_PATH)) {
                out[0] = '\0';
                return;
        }
        snprintf(out, (size_t)out_size, "%s\\FiveM\\FiveM.app\\logs", local);
}

int find_latest_log(char *out, int out_size) {
        char dir[MAX_PATH * 2];
        get_fivem_logs_dir(dir, sizeof(dir));
        if (!dir[0])
                return 0;
        char pattern[MAX_PATH + 64];
        snprintf(pattern, sizeof(pattern), "%.260s\\CitizenFX_log_*.log", dir);
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE)
                return 0;
        char best[MAX_PATH] = "";
        do {
                if (strcmp(fd.cFileName, best) > 0)
                        snprintf(best, sizeof(best), "%s", fd.cFileName);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        snprintf(out, (size_t)out_size, "%s\\%s", dir, best);
        return 1;
}

char *parse_log_file(const char *path, int remove_timestamps) {
        FILE *f = fopen(path, "r");
        if (!f)
                return NULL;
        size_t cap = 512 * 1024;
        char *out = (char *)malloc(cap);
        size_t outlen = 0;
        if (!out) {
                fclose(f);
                return NULL;
        }
        out[0] = '\0';
        char line[8192];
        while (fgets(line, sizeof(line), f)) {
                char *chat = strstr(line, "[chat] ");
                if (!chat)
                        continue;
                chat += 7;
                if (*chat == ' ')
                        chat++;
                char msg[8192];
                strncpy(msg, chat, sizeof(msg) - 1);
                msg[sizeof(msg) - 1] = '\0';
                if (!rtrim_newline(msg))
                        continue;
                strip_color_codes(msg);
                if (!msg[0])
                        continue;
                char entry[8320];
                if (remove_timestamps) {
                        snprintf(entry, sizeof(entry), "%s\r\n", msg);
                } else {
                        char ts[64] = "";
                        if (line[0] == '[') {
                                char *end = strchr(line, ']');
                                if (end) {
                                        char numstr[64] = "";
                                        int nlen = (int)(end - line) - 1;
                                        if (nlen > 0 && nlen < (int)sizeof(numstr)) {
                                                memcpy(numstr, line + 1, (size_t)nlen);
                                                numstr[nlen] = '\0';
                                                long ms = atol(numstr);
                                                long total_sec = ms / 1000;
                                                int h = (int)(total_sec / 3600);
                                                int m = (int)((total_sec % 3600) / 60);
                                                int s = (int)(total_sec % 60);
                                                snprintf(ts, sizeof(ts), "[%02d:%02d:%02d]", h, m, s);
                                        }
                                }
                        }
                        snprintf(entry, sizeof(entry), "%s %s\r\n", ts, msg);
                }
                size_t elen = strlen(entry);
                if (outlen + elen + 1 >= cap) {
                        cap = (cap + elen + 1) * 2;
                        char *tmp = (char *)realloc(out, cap);
                        if (!tmp)
                                break;
                        out = tmp;
                }
                memcpy(out + outlen, entry, elen + 1);
                outlen += elen;
        }
        fclose(f);
        return out;
}
