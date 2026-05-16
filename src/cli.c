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
#include "color.h"
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
          "  cast --help          show this message\n"
          "  cast --version       show version\n",
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

    // collect unique output directories from targets
    for (size_t i = 0; i < cfg.target_count; i++) {
        const char *out = cfg.targets[i].out;
        if (strcmp(out, ".") == 0 || strcmp(out, "..") == 0 || strcmp(out, "src") == 0 ||
            strcmp(out, "/") == 0) {
            fprintf(stderr, "cast: clean refused to delete '%s'\n", out);
            config_free(&cfg);
            return 1;
        }
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", out);
        system(cmd);
    }

    config_free(&cfg);
    printf("cast: cleaned\n");
    return 0;
}

// cast build
[[nodiscard]] static int cmd_build(int argc, char *argv[]) {
    BuildProfile profile = PROFILE_DEBUG;
    const char *target_name = nullptr;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--release") == 0) {
            profile = PROFILE_RELEASE;
        } else if (argv[i][0] != '-') {
            target_name = argv[i];
        } else {
            fprintf(stderr, "cast: unknown flag '%s'\n", argv[i]);
            return 1;
        }
    }

    CastConfig cfg;
    if (!load_config(&cfg)) {
        return 1;
    }
    bool ok = build_run(&cfg, profile, target_name);
    config_free(&cfg);
    return ok ? 0 : 1;
}

// cast run
[[nodiscard]] static int cmd_run(int argc, char *argv[]) {
    const char *target_name = nullptr;
    int run_argc = argc;
    char **run_argv = argv;

    // first non-flag arg is target name
    if (argc > 0 && argv[0][0] != '-') {
        target_name = argv[0];
        run_argc--;
        run_argv++;
    }

    CastConfig cfg;
    if (!load_config(&cfg)) {
        return 1;
    }

    bool ok = build_run(&cfg, PROFILE_DEBUG, target_name);
    if (!ok) {
        config_free(&cfg);
        return 1;
    }

    // find target to get output path
    const char *name = nullptr;
    const char *out = nullptr;
    for (size_t i = 0; i < cfg.target_count; i++) {
        if (!target_name || strcmp(cfg.targets[i].name, target_name) == 0) {
            name = cfg.targets[i].name;
            out = cfg.targets[i].out;
            break;
        }
    }

    char binpath[512];
    snprintf(binpath, sizeof(binpath), "./%s/debug/%s", out, name);
    config_free(&cfg);

    char **exec_argv = malloc(((size_t) run_argc + 2) * sizeof(char *));
    exec_argv[0] = binpath;
    for (int i = 0; i < run_argc; i++) {
        exec_argv[i + 1] = run_argv[i];
    }
    exec_argv[run_argc + 1] = nullptr;

    execv(binpath, exec_argv);
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

    // build
    if (!build_run(&cfg, PROFILE_RELEASE, nullptr)) {
        config_free(&cfg);
        return 1;
    }

    // construct src and dst paths
    char src[512], dst[512];
    for (size_t i = 0; i < cfg.target_count; i++) {
        // skip static
        if (cfg.targets[i].type == TARGET_STATIC) {
            continue;
        }

        snprintf(src, sizeof(src), "%s/release/%s", cfg.targets[i].out, cfg.targets[i].name);
        snprintf(dst, sizeof(dst), "%s/bin/%s", cfg.install.prefix, cfg.targets[i].name);

        char cmd[1200];
        snprintf(cmd, sizeof(cmd), "install -m 755 %s %s", src, dst);
        if (system(cmd) != 0) {
            fprintf(stderr, "cast: install failed for target '%s'\n", cfg.targets[i].name);
            config_free(&cfg);
            return 1;
        }
        printf(COL_BOLD COL_GREEN "cast: " COL_RESET "installed %s to %s\n", cfg.targets[i].name,
               dst);
    }
    config_free(&cfg);
    return 0;
}

// cast init
[[nodiscard]] static int cmd_init(int argc, char *argv[]) {
    const char *name = (argc > 0) ? argv[0] : nullptr;
    return init_run(name) ? 0 : 1;
}

static void cmd_version(void) {
    printf("%s version %s\n", CAST_NAME, CAST_VERSION);
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
    if (strcmp(cmd, "--version") == 0 || strcmp(cmd, "-v") == 0) {
        cmd_version();
        return 0;
    }

    if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        usage();
        return 0;
    }

    fprintf(stderr, "cast: unknown command '%s'\n", cmd);
    usage();
    return 1;
}
