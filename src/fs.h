#pragma once

#include "strbuf.h"
#include <stdbool.h>
#include <stddef.h>

// array of file paths
typedef struct {
    char **paths;
    size_t count;
    size_t cap;
} FileList;

void fl_init(FileList *fl);
void fl_free(FileList *fl);
void fl_add(FileList *fl, const char *path);

// walk dir recursively, collecting files with ext
[[nodiscard]] bool fs_walk(const char *dir, const char *ext, FileList *out);

[[nodiscard]] bool fs_exists(const char *path);
[[nodiscard]] bool fs_mkdir_p(const char *path);

void path_join(StrBuf *sb, const char *a, const char *b);

// return ptr to path
const char *path_basename(const char *path);
const char *path_ext(const char *path);