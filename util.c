#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void strip_color_codes(char *s) {
        int j = 0;
        for (int i = 0; s[i]; i++) {
                if (s[i] == '^') {
                        char nx = s[i + 1];
                        if (nx == '#') {
                                int k = i + 2, h = 0;
                                while (s[k] && h < 6 && ((s[k] >= '0' && s[k] <= '9') || (s[k] >= 'a' && s[k] <= 'f') || (s[k] >= 'A' && s[k] <= 'F'))) {
                                        k++; h++;
                                }
                                if (h == 6) {
                                        i = k - 1;
                                        continue;
                                }
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
                s[j++] = s[i];
        }
        s[j] = '\0';
}

const char *stristr(const char *h, const char *n) {
        if (!*n) return h;
        for (; *h; h++) {
                const char *a = h, *b = n;
                while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
                        a++; b++;
                }
                if (!*b) return h;
        }
        return NULL;
}
