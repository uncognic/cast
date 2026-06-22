#!/bin/sh
cc -std=c23 -Iinclude -Iextern -DCAST_VERSION=\"dev\" -DCAST_NAME=\"cast\" -o cast \
    src/*.c \
    extern/tomlc17.c

