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

#include "config.h"
#include <stdbool.h>

#define CAST_DEPS_DIR ".cast/deps"

// resolve deps in cfg
// clone
// checkout tag
// cast build --release in dep dir
// return false if a fail
[[nodiscard]] bool deps_resolve(const CastConfig *cfg);

// return the path to a dep's .a
const char *dep_lib_path(const CastDep *dep, char *buf, size_t bufsz);

// return path to dep's public header(s)
// .cast/deps/<name>/public/ or <path>/public/
const char *dep_include_path(const CastDep *dep, char *buf, size_t bufsz);
