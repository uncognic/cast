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

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum { TARGET_EXECUTABLE, TARGET_STATIC } TargetType;

typedef struct {
    char name[128];
    char version[32];
    char std[16];
    char compiler[64];
} CastPackage;

typedef struct {
    TargetType type;
    char name[128];
    char **src; // array of source file patterns
    size_t src_count;
    char **include; // array of include directories
    size_t include_count;
    char **links; // array of linked libraries
    size_t link_count;
    char out[128]; // output directory
} CastTarget;

typedef struct {
    char **flags;
    size_t flag_count;
} CastProfile;

typedef struct {
    char prefix[256]; // default /usr/local
} CastInstall;

typedef struct {
    CastPackage package;
    CastTarget *targets;
    size_t target_count;
    CastProfile debug;
    CastProfile release;
    CastInstall install;
} CastConfig;

[[nodiscard]] bool config_load(const char *path, CastConfig *out);
void config_free(CastConfig *cfg);
void config_dump(const CastConfig *cfg); // --debug