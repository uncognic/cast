CC = clang
CFLAGS = -std=c23 -Wall -Wextra -Wpedantic -Isrc
DBGFLAGS = -g -fsanitize=address,undefined
REFLAGS = -O3 -DNDEBUG

SRC = $(shell find src -name '*.c')
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