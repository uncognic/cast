#pragma once

#include <stdarg.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

void sb_init(StrBuf *sb);
void sb_free(StrBuf *sb);
void sb_clear(StrBuf *sb);

void sb_append(StrBuf *sb, const char *s);
void sb_appendc(StrBuf *sb, char c);
void sb_appendf(StrBuf *sb, const char *fmt, ...);