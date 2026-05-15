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

#include "build.h"
#include "color.h"
#include "fs.h"
#include "strbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// extract the base directory from a pattern like src/** /*.c
// take everything before * and strip
static void glob_basedir(const char *pattern, char *buf, size_t buf_size) {
    const char *star = strchr(pattern, '*');
    // no star, use the whole pattern
    if (!star) {
        snprintf(buf, buf_size, "%s", pattern);
        return;
    }

    size_t len = (size_t) (star - pattern);
    if (len >= buf_size) {
        len = buf_size - 1;
    }
    memcpy(buf, pattern, len);
    buf[len] = '\0';

    // strip trailing slash
    while (len > 0 && buf[len - 1] == '/') {
        buf[--len] = '\0';
    }

    if (len == 0) {
        // if the base dir is empty, use current dir
        snprintf(buf, buf_size, ".");
    }
}

// extract file extension
static const char *glob_ext(const char *pattern) {
    const char *dot = strrchr(pattern, '.');
    return dot ? dot : "";
}

// collect source files matching pattern
[[nodiscard]] static bool collect_sources(const CastConfig *cfg, FileList *out) {
    fl_init(out);

    if (cfg->build.src_count == 0) {
        // default to src/*.c
        return fs_walk("src", ".c", out);
    }

    for (size_t i = 0; i < cfg->build.src_count; i++) {
        char basedir[1024];
        glob_basedir(cfg->build.src[i], basedir, sizeof(basedir));
        const char *ext = glob_ext(cfg->build.src[i]);

        if (!fs_walk(basedir, ext, out)) {
            return false;
        }
    }

    return true;
}

bool build_run(const CastConfig *cfg, BuildProfile profile) {
    // collect sources
    FileList sources;
    if (!collect_sources(cfg, &sources)) {
        fl_free(&sources);
        return false;
    }

    if (sources.count == 0) {
        fprintf(stderr, "cast: no source files found\n");
        fl_free(&sources);
        return false;
    }

    // ensure output exists
    if (!fs_mkdir_p(cfg->build.out)) {
        fl_free(&sources);
        return false;
    }

    // profile flags
    const CastProfile *prof = (profile == PROFILE_RELEASE) ? &cfg->release : &cfg->debug;

    // construct binary path <out>/<name>
    StrBuf binpath = {0};
    sb_init(&binpath);
    path_join(&binpath, cfg->build.out, cfg->package.name);

    // build compiler command
    StrBuf cmd = {0};
    sb_init(&cmd);

    sb_append(&cmd, "clang");
    sb_appendf(&cmd, " -std=%s", cfg->package.std);
    sb_append(&cmd, " -Wall -Wextra");

    // include paths
    for (size_t i = 0; i < cfg->build.include_count; i++) {
        sb_appendf(&cmd, " -I%s", cfg->build.include[i]);
    }

    // profile flags
    for (size_t i = 0; i < prof->flag_count; i++) {
        sb_appendf(&cmd, " %s", prof->flags[i]);
    }

    // source files
    for (size_t i = 0; i < sources.count; i++) {
        sb_appendf(&cmd, " %s", sources.paths[i]);
    }

    sb_appendf(&cmd, " -o %s", binpath.data);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int ret = system(cmd.data);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    sb_free(&cmd);
    sb_free(&binpath);
    fl_free(&sources);

    if (ret != 0) {
        fprintf(stderr, COL_RED COL_BOLD "cast:" COL_RESET " build failed (exit %d)\n", ret);
        return false;
    }

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf(COL_GREEN COL_BOLD "cast:" COL_RESET " built %s/%s in %.2fs\n", cfg->build.out,
           cfg->package.name, elapsed);
    return true;
}