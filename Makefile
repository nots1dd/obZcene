###############################################################################
# Build type (debug or release)
###############################################################################
BUILD ?= release

ifeq ($(BUILD),debug)
    CFLAGS += -O0 -g -DDEBUG
    BUILD_DIR := build/debug
else
    CFLAGS += -O2 -DNDEBUG
    BUILD_DIR := build/release
endif

BIN_DIR  := $(BUILD_DIR)
OBJ_DIR  := build/obj
DEP_DIR  := build/deps

###############################################################################
# Parallel build detection
###############################################################################
JOBS := $(shell \
    nproc 2>/dev/null || \
    sysctl -n hw.ncpu 2>/dev/null || \
    echo 4 \
)

###############################################################################
# System Detection
###############################################################################
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
UNAME_R := $(shell uname -r)
UNAME_V := $(shell uname -v)
UNAME_P := $(shell uname -p 2>/dev/null)

KERNEL_BITS := $(shell getconf LONG_BIT)
HOSTNAME    := $(shell hostname)

ENDIAN_CHECK := $(shell printf 'I' | od -to2 | head -1 | awk '{print $$2}')
ifeq ($(ENDIAN_CHECK),000111)
    ENDIAN := little
else
    ENDIAN := big
endif

###############################################################################
# Platform-specific flags
###############################################################################
PLATFORM_FLAGS :=

ifeq ($(UNAME_S),Linux)
    PLATFORM_FLAGS += -D_GNU_SOURCE
endif

ifeq ($(UNAME_S),Darwin)
    PLATFORM_FLAGS += -D_DARWIN_C_SOURCE -mmacosx-version-min=10.14
endif

ifeq ($(UNAME_S),FreeBSD)
    PLATFORM_FLAGS += -D_BSD_SOURCE
endif

ifneq (,$(findstring MINGW,$(UNAME_S)))
    PLATFORM_FLAGS += -D_WIN32
endif

###############################################################################
# Compiler detection
###############################################################################
CC := $(shell command -v gcc 2>/dev/null || command -v clang 2>/dev/null || echo cc)
C_VERSION := $(shell $(CC) --version | head -n 1)
C_TARGET  := $(shell $(CC) -dumpmachine 2>/dev/null)

###############################################################################
# SDL2 detection
###############################################################################
SDL_CFLAGS  := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LDFLAGS := $(shell sdl2-config --libs 2>/dev/null)

ifeq ($(SDL_CFLAGS),)
    SDL_CFLAGS := -I/usr/include/SDL2
    SDL_LDFLAGS := -lSDL2
endif

###############################################################################
# Project settings
###############################################################################
TARGET := main

SRC := src/main.c src/SDL/core.c src/SDL/Render/render.c src/SDL/Textures/textures.c src/SDL/Camera/camera.c

OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEPS := $(patsubst %.c,$(DEP_DIR)/%.d,$(SRC))

INCLUDES := -I include/obZcene/ -I external/
LINKS := -lm -lSDL2_image
CFLAGS += -Wall -Wextra -mfma -mavx2 -march=native $(INCLUDES) $(LINKS) $(SDL_CFLAGS) $(PLATFORM_FLAGS)
LDFLAGS := $(SDL_LDFLAGS)

STRIP := strip

###############################################################################
# Build messages
###############################################################################
QUIET_CC   = @printf "CC      %-30s\n" "$@" && $(CC)
QUIET_LD   = @printf "LD      %-30s\n" "$(BIN_DIR)/$(TARGET)" && $(CC)
QUIET_STRIP= @printf "STRIP   %-30s\n" "$(BIN_DIR)/$(TARGET)" && $(STRIP)

###############################################################################
# Info dump
###############################################################################
define SYSTEM_DUMP

===================== SYSTEM DETECTION =====================
Host Name        : $(HOSTNAME)
OS Name          : $(UNAME_S)
OS Release       : $(UNAME_R)
OS Version       : $(UNAME_V)
Architecture     : $(UNAME_M)
Processor        : $(UNAME_P)
Word Size        : $(KERNEL_BITS)
Endianness       : $(ENDIAN)
============================================================

===================== COMPILER DETECTION ===================
Compiler         : $(CC)
Compiler Version : $(C_VERSION)
Target Triple    : $(C_TARGET)
============================================================

===================== SDL2 DETECTION ========================
SDL CFLAGS       : $(SDL_CFLAGS)
SDL LDFLAGS      : $(SDL_LDFLAGS)
============================================================

===================== PARALLEL BUILD ========================
Jobs Detected    : $(JOBS)
============================================================

endef
export SYSTEM_DUMP

###############################################################################
# Rules
###############################################################################
all: info $(BIN_DIR)/$(TARGET)

info:
	@echo "$$SYSTEM_DUMP"

# Ensure dirs exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/src/SDL
	mkdir -p $(OBJ_DIR)/src

$(DEP_DIR):
	mkdir -p $(DEP_DIR)/src/SDL
	mkdir -p $(DEP_DIR)/src

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link
$(BIN_DIR)/$(TARGET): $(OBJ_DIR) $(DEP_DIR) $(BIN_DIR) $(OBJ)
	$(QUIET_LD) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
ifeq ($(BUILD),release)
	$(QUIET_STRIP) $@
endif

# Compile + dependency file generation
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@) $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(QUIET_CC) $(CFLAGS) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

clean:
	rm -rf build

rebuild: clean all

# include .d files
-include $(DEPS)

###############################################################################
# Code formatting
###############################################################################
FORMAT_DIRS := src include
FORMAT_FILES := $(shell find $(FORMAT_DIRS) -type f \( -name "*.c" -o -name "*.h" \))

format:
	@echo "Formatting source files..."
	@for f in $(FORMAT_FILES); do \
		printf "FMT     %s\n" "$$f"; \
		clang-format -i "$$f"; \
	done

.PHONY: all clean info rebuild
