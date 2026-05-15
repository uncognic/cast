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
#include <unistd.h>

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
[[nodiscard]] static bool collect_sources(const CastTarget *target, FileList *out) {
    fl_init(out);

    if (target->src_count == 0) {
        return fs_walk("src", ".c", out);
    }

    for (size_t i = 0; i < target->src_count; i++) {
        char basedir[1024];
        glob_basedir(target->src[i], basedir, sizeof(basedir));
        const char *ext = glob_ext(target->src[i]);
        if (!fs_walk(basedir, ext, out)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] static bool write_compile_commands(const CastConfig *cfg, const FileList *sources,
                                                 const StrBuf *base_flags) {
    // get current working dir
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd))) {
        perror("cast: getcwd");
        return false;
    }

    // open compile_commands.json for writing
    FILE *f = fopen("compile_commands.json", "w");
    if (!f) {
        fprintf(stderr, "cast: cannot write compile_commands.json\n");
        return false;
    }

    // write JSON array of compile commands
    fputs("[\n", f);
    for (size_t i = 0; i < sources->count; i++) {
        fprintf(f,
                "  {\n"
                "    \"directory\": \"%s\",\n"
                "    \"command\": \"%s %s\",\n"
                "    \"file\": \"%s/%s\"\n"
                "  }%s\n",
                cwd, cfg->package.compiler, base_flags->data, cwd, sources->paths[i],
                i + 1 < sources->count ? "," : "");
    }
    fputs("]\n", f);
    fclose(f);
    return true;
}

bool build_run(const CastConfig *cfg, BuildProfile profile, const char *target_name) {
    const CastProfile *prof = (profile == PROFILE_RELEASE) ? &cfg->release : &cfg->debug;
    bool any = false;

    for (size_t t = 0; t < cfg->target_count; t++) {
        const CastTarget *target = &cfg->targets[t];

        // skip if target_name is given and doesn't match this target
        if (target_name && strcmp(target->name, target_name) != 0) {
            continue;
        }

        any = true;

        FileList sources;
        if (!collect_sources(target, &sources)) {
            fl_free(&sources);
            return false;
        }

        if (sources.count == 0) {
            fprintf(stderr, "cast: no source files found for target '%s'\n", target->name);
            fl_free(&sources);
            return false;
        }

        if (!fs_mkdir_p(target->out)) {
            fl_free(&sources);
            return false;
        }

        StrBuf binpath = {0};
        sb_init(&binpath);
        path_join(&binpath, target->out, target->name);

        StrBuf cmd = {0};
        sb_init(&cmd);

        sb_append(&cmd, cfg->package.compiler);
        sb_appendf(&cmd, " -std=%s", cfg->package.std);
        sb_append(&cmd, " -Wall -Wextra");

        for (size_t i = 0; i < target->include_count; i++) {
            sb_appendf(&cmd, " -I%s", target->include[i]);
        }
        for (size_t i = 0; i < prof->flag_count; i++) {
            sb_appendf(&cmd, " %s", prof->flags[i]);
        }
        for (size_t i = 0; i < sources.count; i++) {
            sb_appendf(&cmd, " %s", sources.paths[i]);
        }

        sb_appendf(&cmd, " -o %s", binpath.data);

        /* compile_commands.json */
        StrBuf base_flags = {0};
        sb_init(&base_flags);
        sb_appendf(&base_flags, "-std=%s", cfg->package.std);
        sb_append(&base_flags, " -Wall -Wextra");
        for (size_t i = 0; i < target->include_count; i++) {
            sb_appendf(&base_flags, " -I%s", target->include[i]);
        }
        for (size_t i = 0; i < prof->flag_count; i++) {
            sb_appendf(&base_flags, " %s", prof->flags[i]);
        }
        bool result = write_compile_commands(cfg, &sources, &base_flags);
        if (!result) {
            fprintf(stderr, "cast: failed to write compile_commands.json\n");
            sb_free(&base_flags);
            fl_free(&sources);
            return false;
        }
        sb_free(&base_flags);

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
        printf(COL_GREEN COL_BOLD "cast:" COL_RESET " built %s/%s in %.2fs\n", target->out,
               target->name, elapsed);
    }

    if (!any) {
        fprintf(stderr, "cast: no target named '%s'\n", target_name);
        return false;
    }

    return true;
}