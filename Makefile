# Makefile for obZcene

###############################################################################
# Project configuration (user overridable)
###############################################################################
TARGET        ?= main
BUILD         ?= release
PREFIX        ?= /usr/local
VERBOSE       ?= 0
JOBS          ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

###############################################################################
# Derived directories
###############################################################################
BUILD_DIR     := build/$(BUILD)
OBJ_DIR       := build/obj/$(BUILD)
DEP_DIR       := build/deps/$(BUILD)
BIN_DIR       := $(BUILD_DIR)

###############################################################################
# Detect host
###############################################################################
UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
UNAME_R := $(shell uname -r 2>/dev/null || echo Unknown)
UNAME_M := $(shell uname -m 2>/dev/null || echo Unknown)
HOSTNAME := $(or $(shell hostname 2>/dev/null), $(shell uname -n 2>/dev/null), unknown)
ENDIAN := $(shell printf 'I' | od -An -to2 | awk '{print ($$1=="000111")?"little":"big"}')

###############################################################################
# Compiler
###############################################################################
CC        := $(shell command -v gcc || command -v clang || echo cc)
C_VERSION := $(shell $(CC) --version | head -n1)
C_TARGET  := $(shell $(CC) -dumpmachine 2>/dev/null || echo unknown)

###############################################################################
# SDL2
###############################################################################
SDL_CFLAGS  := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LDFLAGS := $(shell sdl2-config --libs 2>/dev/null)
ifeq ($(SDL_CFLAGS),)
    SDL_CFLAGS := -I/usr/include/SDL2
    SDL_LDFLAGS := -lSDL2
endif

###############################################################################
# Flags
###############################################################################
INCLUDES := -I include/obZcene -I external
COMMON_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter $(INCLUDES) -lm $(SDL_CFLAGS) -march=native
LDFLAGS := $(SDL_LDFLAGS) -lSDL2_image

ifeq ($(BUILD),debug)
    COMMON_CFLAGS += -O0 -g -DDEBUG
else
    COMMON_CFLAGS += -O2 -DNDEBUG
endif

###############################################################################
# Source files
###############################################################################
SRC := \
    src/main.c \
    src/SDL/core.c \
    src/SDL/Render/render.c \
    src/SDL/Textures/textures.c \
    src/SDL/Camera/camera.c \
    src/Utils/vec.c

OBJ  := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))
DEPS := $(patsubst %.c,$(DEP_DIR)/%.d,$(SRC))

###############################################################################
# Colored output helpers
###############################################################################
COLOR_CC  := \033[1;32m
COLOR_LD  := \033[1;36m
COLOR_RST := \033[0m

# Always print the simple message, never the full command
ECHO_CC_CMD = @printf "$(COLOR_CC)CC$(COLOR_RST)  %-30s\n" "$@"
ECHO_LD_CMD = @printf "$(COLOR_LD)LD$(COLOR_RST)  %-30s\n" "$@"

ifeq ($(VERBOSE),1)
    COMPILE = $(CC) $(COMMON_CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@
    LINK    = $(CC) $(COMMON_CFLAGS) $(OBJ) -o $@ $(LDFLAGS)
else
    COMPILE = @$(CC) $(COMMON_CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@
    LINK    = @$(CC) $(COMMON_CFLAGS) $(OBJ) -o $@ $(LDFLAGS)
endif

###############################################################################
# Rules
###############################################################################
all: info $(BIN_DIR)/$(TARGET)

# Info display with single-line separator
info:
	@SEP=$(shell locale 2>/dev/null | grep -iqE "UTF-8|utf8" && echo "-" || echo "-"); \
	TL=$(shell [ "$$SEP" = "-" ] && echo "+" || echo "┌"); \
	TR=$(shell [ "$$SEP" = "-" ] && echo "+" || echo "┐"); \
	BL=$(shell [ "$$SEP" = "-" ] && echo "+" || echo "└"); \
	BR=$(shell [ "$$SEP" = "-" ] && echo "+" || echo "┘"); \
	VB=$(shell [ "$$SEP" = "-" ] && echo "|" || echo "│"); \
	WIDTH=65; \
	printf "%s%s%s\n" "$$TL" "$$(printf '%*s' $$WIDTH '' | tr ' ' $$SEP)" "$$TR"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " OBZCENE BUILD INFO " "$$VB"; \
	printf "%s%s%s\n" "$$VB" "$$(printf '%*s' $$WIDTH '' | tr ' ' ' ')" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Host: $(HOSTNAME)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " OS: $(UNAME_S) $(UNAME_R)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Arch: $(UNAME_M)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Endianness: $(ENDIAN)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Compiler: $(CC)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Version: $(C_VERSION)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Target triple: $(C_TARGET)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " SDL CFLAGS: $(SDL_CFLAGS)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " SDL LDFLAGS: $(SDL_LDFLAGS)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Build type: $(BUILD)" "$$VB"; \
	printf "%s%-*s%s\n" "$$VB" $$WIDTH " Jobs: $(JOBS)" "$$VB"; \
	printf "%s%s%s\n\n" "$$BL" "$$(printf '%*s' $$WIDTH '' | tr ' ' $$SEP)" "$$BR"

# Ensure directories exist
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Build objects
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR) $(DEP_DIR)
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEP_DIR)/$*.d)
	$(ECHO_CC_CMD)
	$(COMPILE)

# Build target
$(BIN_DIR)/$(TARGET): $(OBJ) | $(BIN_DIR)
	$(ECHO_LD_CMD)
	$(LINK)

# Clean
clean:
	rm -rf build

# Rebuild
rebuild: clean all

# Install/uninstall
install: $(BIN_DIR)/$(TARGET)
	install -Dm755 $(BIN_DIR)/$(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

# Include dependency files
-include $(DEPS)

.PHONY: all clean rebuild info install uninstall
