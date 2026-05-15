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

#include "cli.h"
#include "build.h"
#include "config.h"
#include "fs.h"
#include "init.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CAST_TOML "cast.toml"

static void usage(void) {
    fputs("cast - a build tool for C\n"
          "\n"
          "usage:\n"
          "  cast init [name]     scaffold a new project\n"
          "  cast build           build (debug)\n"
          "  cast build --release build (release)\n"
          "  cast clean           remove build directory\n"
          "  cast run [args...]   build and run\n"
          "  cast install         install binary to prefix\n"
          "  cast --help          show this message\n",
          stderr);
}

[[nodiscard]] static bool load_config(CastConfig *cfg) {
    if (!fs_exists(CAST_TOML)) {
        fprintf(stderr, "cast: no cast.toml found in current directory\n");
        return false;
    }
    return config_load(CAST_TOML, cfg);
}

[[nodiscard]] static int cmd_clean(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    CastConfig cfg;
    if (!load_config(&cfg)) {
        return 1;
    }

    // don't delete some stuff
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", cfg.build.out);
    if (strcmp(dir, ".") == 0 || strcmp(dir, "..") == 0) {
        fprintf(stderr, "cast: clean failed (refusing to delete %s)\n", dir);
        config_free(&cfg);
        return 1;
    }

    if (strcmp(dir, "src") == 0 || strcmp(dir, "include") == 0) {
        fprintf(stderr, "cast: clean failed (refusing to delete %s)\n", dir);
        config_free(&cfg);
        return 1;
    }

    if (strcmp(dir, "~") == 0 || strcmp(dir, "/home") == 0) {
        fprintf(stderr, "cast: clean failed (refusing to delete %s)\n", dir);
        config_free(&cfg);
        return 1;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", cfg.build.out);
    config_free(&cfg);

    if (system(cmd) != 0) {
        fprintf(stderr, "cast: clean failed\n");
        return 1;
    }

    printf("cast: cleaned\n");
    return 0;
}

// cast build
[[nodiscard]] static int cmd_build(int argc, char *argv[]) {
    BuildProfile profile = PROFILE_DEBUG;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0) {
            profile = PROFILE_RELEASE;
        } else {
            fprintf(stderr, "cast: unknown flag '%s'\n", argv[i]);
            return 1;
        }
    }

    CastConfig cfg;
    if (!load_config(&cfg)) {
        return 1;
    }

    bool ok = build_run(&cfg, profile);
    config_free(&cfg);
    return ok ? 0 : 1;
}

// cast run
[[nodiscard]] static int cmd_run(int argc, char *argv[]) {
    CastConfig cfg;
    if (!load_config(&cfg)) {
        return 1;
    }

    bool ok = build_run(&cfg, PROFILE_DEBUG);
    if (!ok) {
        config_free(&cfg);
        return 1;
    }

    // build path to bin
    char binpath[512];
    snprintf(binpath, sizeof(binpath), "./%s/%s", cfg.build.out, cfg.package.name);
    config_free(&cfg);

    // exec bin passing along args
    char **exec_argv = malloc(((size_t) argc + 2) * sizeof(char *));
    exec_argv[0] = binpath;
    for (int i = 0; i < argc; i++) {
        exec_argv[i + 1] = argv[i];
    }
    exec_argv[argc + 1] = nullptr;

    execv(binpath, exec_argv);

    // execv only returns on error
    perror("cast: execv");
    free(exec_argv);
    return 1;
}

[[nodiscard]] static int cmd_install(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    CastConfig cfg;
    if (!load_config(&cfg)) {
        config_free(&cfg);
        return 1;
    }

    // construct src and dst paths
    char src[512], dst[512];
    snprintf(src, sizeof(src), "%s/%s", cfg.build.out, cfg.package.name);
    snprintf(dst, sizeof(dst), "%s/bin/%s", cfg.install.prefix, cfg.package.name);

    char bindir[512];
    snprintf(bindir, sizeof(bindir), "%s/bin", cfg.install.prefix);
    if (!fs_mkdir_p(bindir)) {
        config_free(&cfg);
        return 1;
    }

    // copy binary
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "install -m 755 %s %s", src, dst);
    if (system(cmd) != 0) {
        fprintf(stderr, "cast: install failed (have you tried running as root?)\n");
        config_free(&cfg);
        return 1;
    }

    // set -x
    snprintf(cmd, sizeof(cmd), "chmod 755 %s", dst);
    system(cmd);

    printf("cast: installed to %s\n", dst);
    config_free(&cfg);
    return 0;
}

// cast init
[[nodiscard]] static int cmd_init(int argc, char *argv[]) {
    const char *name = (argc > 0) ? argv[0] : nullptr;
    return init_run(name) ? 0 : 1;
}

[[nodiscard]] int cli_run(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    const char *cmd = argv[1];
    // pass rest args
    int rest_argc = argc - 2;
    char **rest = argv + 2;

    if (strcmp(cmd, "build") == 0) {
        return cmd_build(rest_argc, rest);
    }
    if (strcmp(cmd, "run") == 0) {
        return cmd_run(rest_argc, rest);
    }
    if (strcmp(cmd, "install") == 0) {
        return cmd_install(rest_argc, rest);
    }
    if (strcmp(cmd, "init") == 0) {
        return cmd_init(rest_argc, rest);
    }
    if (strcmp(cmd, "clean") == 0) {
        return cmd_clean(rest_argc, rest);
    }

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        usage();
        return 0;
    }

    fprintf(stderr, "cast: unknown command '%s'\n", cmd);
    usage();
    return 1;
}
