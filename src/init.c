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

#include "init.h"
#include "fs.h"
#include "strbuf.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

[[nodiscard]] static bool write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "cast: cannot write '%s': %s\n", path, strerror(errno));
        return false;
    }
    fputs(content, f);
    fclose(f);
    return true;
}

bool init_run(const char *name) {
    StrBuf path = {0};
    sb_init(&path);

    // if name given, enter it and create dir if not exists
    if (name) {
        if (fs_exists(name)) {
            fprintf(stderr, "cast: '%s' already exists\n", name);
            sb_free(&path);
            return false;
        }
        if (!fs_mkdir_p(name)) {
            sb_free(&path);
            return false;
        }
        printf("cast: created directory %s/\n", name);
    }

    const char *base = name ? name : ".";

    // create src and include
    sb_clear(&path);
    path_join(&path, base, "src");
    if (!fs_mkdir_p(path.data)) {
        sb_free(&path);
        return false;
    }

    sb_clear(&path);
    path_join(&path, base, "include");
    if (!fs_mkdir_p(path.data)) {
        sb_free(&path);
        return false;
    }

    // cast.toml
    const char *project_name = name ? name : "myapp";

    char toml[1024];
    snprintf(toml, sizeof(toml),
             "[package]\n"
             "name     = \"%s\"\n"
             "version  = \"0.1.0\"\n"
             "std      = \"c17\"\n"
             "compiler = \"gcc\"\n"
             "\n"
             "[build]\n"
             "src     = [\"src/**/*.c\"]\n"
             "include = [\"include\"]\n"
             "out     = \"build\"\n"
             "\n"
             "[profile.debug]\n"
             "flags = [\"-g\", \"-fsanitize=address,undefined\", \"-O0\"]\n"
             "\n"
             "[profile.release]\n"
             "flags = [\"-O3\", \"-DNDEBUG\"]\n"
             "\n"
             "[install]\n"
             "prefix = \"/usr/local\"\n",
             project_name);

    sb_clear(&path);
    path_join(&path, base, "cast.toml");
    if (!write_file(path.data, toml)) {
        sb_free(&path);
        return false;
    }

    // src/main.c
    char main_c[512];
    snprintf(main_c, sizeof(main_c),
             "#include <stdio.h>\n"
             "\n"
             "int main(void) {\n"
             "    puts(\"Hello, World!\");\n"
             "    return 0;\n"
             "}\n");

    sb_clear(&path);
    path_join(&path, base, "src/main.c");
    if (!write_file(path.data, main_c)) {
        sb_free(&path);
        return false;
    }

    // .gitignore
    sb_clear(&path);
    path_join(&path, base, ".gitignore");
    if (!write_file(path.data, "build/\n")) {
        sb_free(&path);
        return false;
    }

    sb_free(&path);

    printf("cast: initialised project '%s'\n", project_name);
    printf("      cast build    to compile\n");
    printf("      cast run      to run\n");

    return true;
}