/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "strbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SB_INIT_CAP 64

static void sb_grow(StrBuf *sb, size_t needed) {
    // if the current capacity is enough, do nothing
    if (sb->len + needed < sb->cap) {
        return;
    }

    // double capacity until it's enough
    while (sb->cap < sb->len + needed) {
        sb->cap *= 2;
    }

    // reallocate the buffer
    sb->data = realloc(sb->data, sb->cap);

    // if realloc fails
    if (!sb->data) {
        fprintf(stderr, "out of memory for strbuf\n");
        exit(1);
    }
}

// initialize sb
void sb_init(StrBuf *sb) {
    sb->data = malloc(SB_INIT_CAP);
    sb->len = 0;
    sb->cap = SB_INIT_CAP;

    // if malloc fails
    if (!sb->data) {
        fprintf(stderr, "out of memory for strbuf\n");
        exit(1);
    }
}

// free sb
void sb_free(StrBuf *sb) {
    free(sb->data);
    sb->data = nullptr;
    sb->len = 0;
    sb->cap = 0;
}

// clear sb
void sb_clear(StrBuf *sb) {
    sb->len = 0;
    sb->data[0] = '\0';
}

void sb_append(StrBuf *sb, const char *s) {
    size_t slen = strlen(s);
    
    sb_grow(sb, slen + 1);
    memcpy(sb->data + sb->len, s, slen + 1);
    sb->len += slen;
}

void sb_appendc(StrBuf *sb, char c) {
    sb_grow(sb, 2);
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
}

void sb_appendf(StrBuf *sb, const char *fmt, ...) {
    va_list args;

    // first pass measure
    va_start(args, fmt);
    int needed = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (needed < 0) {
        return;
    }

    sb_grow(sb, (size_t)needed + 1);

    // second pass write
    va_start(args, fmt);
    vsnprintf(sb->data + sb->len, (size_t)needed + 1, fmt, args);
    va_end(args);

    sb->len += (size_t)needed;
}