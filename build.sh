#!/bin/sh
cc -std=c23 -Iinclude -Iextern -D_POSIX_C_SOURCE=200809L -o cast \
    src/*.c \
    extern/tomlc17.c

