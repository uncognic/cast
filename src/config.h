#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char name[128];
    char version[32];
    char std[16];
} CastPackage;

typedef struct {
    char **src; // array of source file paths
    size_t src_count;
    char **include; // array of include paths
    size_t include_count;
    char out[128]; // output path, default build
} CastBuild;

typedef struct {
    char **flags;
    size_t flag_count;
} CastProfile;

typedef struct {
    char prefix[256]; // default /usr/local
} CastInstall;

typedef struct {
    CastPackage package;
    CastBuild build;
    CastProfile debug;
    CastProfile release;
    CastInstall install;
} CastConfig;

[[nodiscard]] bool config_load(const char *path, CastConfig *out);
void config_free(CastConfig *cfg);
void config_dump(const CastConfig *cfg); // --debug