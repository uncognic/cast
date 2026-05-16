#!/bin/sh
cc -std=c23 -Iinclude -Iextern -D_POSIX_C_SOURCE=200809L -DCAST_VERSION=\"dev\" -DCAST_NAME=\"cast\" -o cast \
    src/*.c \
    extern/tomlc17.c

