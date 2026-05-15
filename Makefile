CC = clang
DBGFLAGS = -g -fsanitize=address,undefined
REFLAGS = -O3 -DNDEBUG

STD = -std=c23
SRC = $(shell find src -name '*.c') extern/tomlc17.c
CFLAGS = $(STD) -Wall -Wextra -Wpedantic -Isrc -Iextern -D_POSIX_C_SOURCE=200809L
TARGET = cast

.PHONY: all debug release clean

all: debug
debug: CFLAGS += $(DBGFLAGS)
debug: $(TARGET)

release: CFLAGS += $(REFLAGS)
release: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^
 
clean:
	rm -f $(TARGET)