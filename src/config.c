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

#include "config.h"
#include "../extern/tomlc17.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// copy a TOML_STRING to a fixed size buf
static void str_copy(char *dst, size_t dstsz, toml_datum_t d) {
    if (d.type == TOML_STRING) {
        snprintf(dst, dstsz, "%s", d.u.s);
    }
}

// extract array of strings from a TOML array
static char **arr_strings(toml_datum_t d, size_t *count_out) {
    *count_out = 0;
    if (d.type != TOML_ARRAY || d.u.arr.size <= 0) {
        return nullptr;
    }

    int n = d.u.arr.size;
    char **out = malloc((size_t) n * sizeof(char *));
    if (!out) {
        fprintf(stderr, "error: malloc failed\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        toml_datum_t elem = d.u.arr.elem[i];
        out[i] = (elem.type == TOML_STRING) ? strdup(elem.u.s) : strdup("");
    }

    *count_out = (size_t) n;
    return out;
}

static void parse_profile(toml_datum_t t, CastProfile *p) {
    if (t.type != TOML_TABLE) {
        return;
    }
    p->flags = arr_strings(toml_get(t, "flags"), &p->flag_count);
}

bool config_load(const char *path, CastConfig *cfg) {
    memset(cfg, 0, sizeof(*cfg));

    // defaults
    snprintf(cfg->build.out, sizeof(cfg->build.out), "build");
    snprintf(cfg->install.prefix, sizeof(cfg->install.prefix), "/usr/local");
    snprintf(cfg->package.std, sizeof(cfg->package.std), "c17");
    snprintf(cfg->package.compiler, sizeof(cfg->package.compiler), "gcc");

    toml_result_t res = toml_parse_file_ex(path);
    if (!res.ok) {
        fprintf(stderr, "cast: parse error in '%s': %s\n", path, res.errmsg);
        return false;
    }

    toml_datum_t root = res.toptab;

    // [package]
    toml_datum_t pkg = toml_get(root, "package");
    if (pkg.type == TOML_TABLE) {
        str_copy(cfg->package.name, sizeof(cfg->package.name), toml_get(pkg, "name"));
        str_copy(cfg->package.version, sizeof(cfg->package.version), toml_get(pkg, "version"));
        str_copy(cfg->package.std, sizeof(cfg->package.std), toml_get(pkg, "std"));
        str_copy(cfg->package.compiler, sizeof(cfg->package.compiler), toml_get(pkg, "compiler"));
    }

    // [build]
    toml_datum_t build = toml_get(root, "build");
    if (build.type == TOML_TABLE) {
        cfg->build.src = arr_strings(toml_get(build, "src"), &cfg->build.src_count);
        cfg->build.include = arr_strings(toml_get(build, "include"), &cfg->build.include_count);
        str_copy(cfg->build.out, sizeof(cfg->build.out), toml_get(build, "out"));
    }

    // [profile.debug] [profile.release]
    parse_profile(toml_seek(root, "profile.debug"), &cfg->debug);
    parse_profile(toml_seek(root, "profile.release"), &cfg->release);

    // [install]
    toml_datum_t install = toml_get(root, "install");
    if (install.type == TOML_TABLE) {
        str_copy(cfg->install.prefix, sizeof(cfg->install.prefix), toml_get(install, "prefix"));
    }

    toml_free(res);
    return true;
}

void config_free(CastConfig *cfg) {
    for (size_t i = 0; i < cfg->build.src_count; i++) {
        free(cfg->build.src[i]);
    }
    for (size_t i = 0; i < cfg->build.include_count; i++) {
        free(cfg->build.include[i]);
    }
    for (size_t i = 0; i < cfg->debug.flag_count; i++) {
        free(cfg->debug.flags[i]);
    }
    for (size_t i = 0; i < cfg->release.flag_count; i++) {
        free(cfg->release.flags[i]);
    }

    free(cfg->build.src);
    free(cfg->build.include);
    free(cfg->debug.flags);
    free(cfg->release.flags);
}

void config_dump(const CastConfig *cfg) {
    printf("[package]\n  name=%s  version=%s  std=%s\n", cfg->package.name, cfg->package.version,
           cfg->package.std);
    printf("[build]\n  out=%s\n", cfg->build.out);
    for (size_t i = 0; i < cfg->build.src_count; i++) {
        printf("  src[%zu]=%s\n", i, cfg->build.src[i]);
    }
    for (size_t i = 0; i < cfg->build.include_count; i++) {
        printf("  include[%zu]=%s\n", i, cfg->build.include[i]);
    }
    printf("[install]\n  prefix=%s\n", cfg->install.prefix);
}