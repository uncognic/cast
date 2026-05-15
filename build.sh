#!/bin/sh
cc -std=c23 -Iinclude -Iextern -D_POSIX_C_SOURCE=200809L -o cast \
    src/strbuf.c src/fs.c src/config.c src/build.c src/cli.c src/init.c src/main.c \
    extern/tomlc17.c
