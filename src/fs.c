#include "fs.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define FL_INIT_CAP 32

// FileList

void fl_init(FileList *fl) {
    fl->paths = malloc(FL_INIT_CAP * sizeof(char));
    fl->count = 0;
    fl->cap = FL_INIT_CAP;
}

void fl_free(FileList *fl) {
    for (size_t i = 0; i < fl->count; i++) {
        free(fl->paths[i]);
    }

    free(fl->paths);
    fl->paths = nullptr;
    fl->count = 0;
    fl->cap = 0;
}

void fl_push(FileList *fl, const char *path) {

    // if there is not enough space
    if (fl->count >= fl->cap) {
        fl->cap *= 2;
        fl->paths = realloc(fl->paths, fl->cap * sizeof(char));
        if (!fl->paths) {
            fprintf(stderr, "cast: out of memory\n");
            exit(1);
        }
    }

    fl->paths[fl->count++] = strdup(path);
}

void path_join(StrBuf *sb, const char *a, const char *b) {
    sb_append(sb, a);
    if (sb->len > 0 && sb->data[sb->len - 1] != '/') {
        sb_appendc(sb, '/');
    }
    sb_append(sb, b);
}

bool fs_walk(const char *dir, const char *ext, FileList *out) {
    DIR *d = opendir(dir);
    if (!d) {
        fprintf(stderr, "cannot open dir '%s': %s\n", dir, strerror(errno));
        return false;
    }

    StrBuf path = {0};
    sb_init(&path);

    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        // skip hidden files and dirs
        if (entry->d_name[0] == '.') {
            continue;
        }

        sb_clear(&path);
        path_join(&path, dir, entry->d_name);

        struct stat st;
        // if stat fails, skip this entry
        if (stat(path.data, &st) != 0) {
            continue;
        }

        // if dir then recurse
        if (S_ISDIR(st.st_mode)) {
            // if walk fails, close dir and return false
            if (!fs_walk(path.data, ext, out)) {
                closedir(d);
                sb_free(&path);
                return false;
            }
            // if regular file and ext matches, add to list
        } else if (S_ISREG(st.st_mode)) {
            if (strcmp(path_ext(path.data), ext) == 0) {
                fl_push(out, path.data);
            }
        }
    }

    sb_free(&path);
    closedir(d);
    return true;
}

bool fs_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool fs_mkdir_p(const char *path) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", path);

    // create parent dirs one by one
    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "cast: mkdir '%s': %s\n", tmp, strerror(errno));
            return false;
        }
        *p = '/';
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "cast: mkdir '%s': %s\n", tmp, strerror(errno));
        return false;
    }

    return true;
}

const char *path_basename(const char *path) {
    const char *s = strrchr(path, '/');
    return s ? s + 1 : path;
}

const char *path_ext(const char *path) {
    const char *base = path_basename(path);
    const char *dot = strrchr(base, '.');
    return dot ? dot : "";
}