# Compiler and flags
CC = gcc

# Detect platform and set target name
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
	TARGET = SDL2_2D_Game.exe
else
	TARGET = SDL2_2D_Game
endif

# SDL2 flags (prefer sdl2-config, fallback to pkg-config)
SDL2_CFLAGS := $(shell (sdl2-config --cflags) 2>/dev/null || pkg-config --cflags sdl2)
SDL2_LIBS   := $(shell (sdl2-config --libs)   2>/dev/null || pkg-config --libs sdl2)
SDL2_IMAGE  := $(shell pkg-config --libs SDL2_image 2>/dev/null || echo -lSDL2_image)
SDL2_MIXER  := $(shell pkg-config --libs SDL2_mixer 2>/dev/null || echo -lSDL2_mixer)
SDL2_TTF    := $(shell pkg-config --libs SDL2_ttf   2>/dev/null || echo -lSDL2_ttf)

CFLAGS = -Wall -g $(SDL2_CFLAGS) -Iinclude -Isrc
LDFLAGS = $(SDL2_LIBS) $(SDL2_IMAGE) $(SDL2_MIXER) $(SDL2_TTF) -lm

# Source files
SRC = main.c \
      src/menu/menu.c \
      src/options/options.c \
      src/story/story.c \
      src/score/score.c \
      src/play/play.c \
      src/save/save.c \
      src/newgame/newgame.c \
      src/single/single.c \
      src/multi/multi.c \
      src/enigm/enigm.c \
      src/quiz/quiz.c \
      src/common.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile .c files to .o files
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Clean build files
clean:
	rm -f $(OBJ) $(TARGET)

# Run (Linux/macOS)
run: $(TARGET)
	./$(TARGET)
