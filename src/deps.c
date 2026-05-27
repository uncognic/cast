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
#define _POSIX_C_SOURCE 200809L
#include "deps.h"
#include "color.h"
#include "fs.h"
#include "strbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *dep_lib_path(const CastDep *dep, char *buf, size_t bufsz) {
    snprintf(buf, bufsz, "%s/%s/build/release/lib%s.a", CAST_DEPS_DIR, dep->name, dep->name);
    return buf;
}

const char *dep_include_path(const CastDep *dep, char *buf, size_t bufsz) {
    snprintf(buf, bufsz, "%s/%s/public", CAST_DEPS_DIR, dep->name);
    return buf;
}

[[nodiscard]] static bool dep_clone(const CastDep *dep, const char *dep_dir) {
    if (dep->path[0] != '\0') {
        if (!fs_exists(dep->path)) {
            fprintf(stderr,
                    COL_RED COL_BOLD "cast:" COL_RESET " local path '%s' for dep '%s' does not exist\n",
                    dep->path, dep->name);
            return false;
        }
        if (fs_exists(dep_dir)) {
            return true;
        }
        printf(COL_BOLD "cast:" COL_RESET " copying dep '%s' from %s\n", dep->name, dep->path);
        StrBuf cmd = {0};
        sb_init(&cmd);
        sb_appendf(&cmd, "cp -r %s %s && rm -rf %s/build", dep->path, CAST_DEPS_DIR, dep_dir);
        int ret = system(cmd.data);
        sb_free(&cmd);
        if (ret != 0) {
            fprintf(stderr, COL_RED COL_BOLD "cast:" COL_RESET " failed to copy dep '%s'\n",
                    dep->name);
            return false;
        }
        return true;
    }

    if (fs_exists(dep_dir)) {
        return true;
    }

    printf(COL_BOLD "cast:" COL_RESET " cloning %s from %s\n", dep->name, dep->git);

    StrBuf cmd = {0};
    sb_init(&cmd);
    sb_appendf(&cmd, "git clone --recurse-submodules %s %s", dep->git, dep_dir);

    int ret = system(cmd.data);
    sb_free(&cmd);

    if (ret != 0) {
        fprintf(stderr, COL_RED COL_BOLD "cast:" COL_RESET " failed to clone '%s'\n", dep->name);
        return false;
    }

    return true;
}

[[nodiscard]] static bool dep_checkout(const CastDep *dep, const char *dep_dir) {
    if (dep->path[0] != '\0' || dep->tag[0] == '\0') {
        return true; // no tag, so stay on default branch
    }

    StrBuf cmd = {0};
    sb_init(&cmd);
    sb_appendf(&cmd, "git -C %s checkout %s", dep_dir, dep->tag);

    int ret = system(cmd.data);
    sb_free(&cmd);

    if (ret != 0) {
        fprintf(stderr,
                COL_RED COL_BOLD "cast:" COL_RESET " failed to checkout '%s' for dep '%s'\n",
                dep->tag, dep->name);
        return false;
    }

    return true;
}

[[nodiscard]] static bool dep_build(const CastDep *dep, const char *dep_dir) {
    char lib[512];
    dep_lib_path(dep, lib, sizeof(lib));

    if (fs_exists(lib)) {
        printf(COL_BOLD "cast:" COL_RESET " dep '%s' already built, skipping\n", dep->name);
        return true;
    }

    printf(COL_BOLD "cast:" COL_RESET " building dep '%s'\n", dep->name);
    fflush(stdout);

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        perror("cast: getcwd");
        return false;
    }
    char abs_dep_dir[4097];
    snprintf(abs_dep_dir, sizeof(abs_dep_dir), "%s/%s", cwd, dep_dir);

    StrBuf cmd = {0};
    sb_init(&cmd);
    sb_appendf(&cmd, "cd %s && cast build --release %s", abs_dep_dir, dep->name);

    int ret = system(cmd.data);
    sb_free(&cmd);

    if (ret != 0) {
        fprintf(stderr, COL_RED COL_BOLD "cast:" COL_RESET " failed to build dep '%s'\n",
                dep->name);
        return false;
    }

    // verify the .a was made
    if (!fs_exists(lib)) {
        fprintf(stderr,
                COL_RED COL_BOLD "cast:" COL_RESET " dep '%s' built but no .a found at '%s'\n"
                                 "       make sure the dep has a static [[target]] named '%s'\n",
                dep->name, lib, dep->name);
        return false;
    }

    return true;
}

bool deps_resolve(const CastConfig *cfg) {
    if (cfg->dep_count == 0) {
        return true;
    }

    // ensure .cast/deps exists
    if (!fs_mkdir_p(CAST_DEPS_DIR)) {
        return false;
    }

    for (size_t i = 0; i < cfg->dep_count; i++) {
        const CastDep *dep = &cfg->deps[i];

        char dep_dir[512];
        snprintf(dep_dir, sizeof(dep_dir), "%s/%s", CAST_DEPS_DIR, dep->name);

        if (!dep_clone(dep, dep_dir)) {
            return false;
        }
        if (!dep_checkout(dep, dep_dir)) {
            return false;
        }
        if (!dep_build(dep, dep_dir)) {
            return false;
        }

        printf(COL_GREEN COL_BOLD "cast:" COL_RESET " dep '%s' ready\n", dep->name);
    }

    return true;
}
