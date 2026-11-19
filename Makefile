# Build My Bank Application (requires raylib 5.x)
#
#   make                       # uses RAYLIB_PATH below
#   make RAYLIB_PATH=C:/raylib/raylib
#
# On Windows the easiest setup is the raylib installer, which ships with
# w64devkit (gcc + make) and puts raylib in C:/raylib.

RAYLIB_PATH ?= C:/raylib/raylib
CC          ?= gcc
CFLAGS      += -std=c99 -Wall -Wextra -O2 -I$(RAYLIB_PATH)/src
TARGET      := bankapp

ifeq ($(OS),Windows_NT)
    TARGET  := bankapp.exe
    LDFLAGS += -L$(RAYLIB_PATH)/src -lraylib -lopengl32 -lgdi32 -lwinmm
else
    LDFLAGS += -lraylib -lm -lpthread -ldl
endif

$(TARGET): src/main.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
