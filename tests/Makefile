# ----------------- Compiler -----------------
CC := $(shell command -v gcc 2>/dev/null || command -v clang 2>/dev/null || echo cc)
INCLUDES := -I ../include/obZcene/
CFLAGS ?= -O3 -msse -mfma -mavx2 -march=native -march=native -Wall -Wextra $(INCLUDES)
LDFLAGS ?= -lm

# ----------------- Test files -----------------
TEST_DIR := ./bin
TEST_SRCS := obz_math_sqrt.test.c obz_math_trig.test.c   # add more test source files here
TEST_BINS := $(patsubst %.c,$(TEST_DIR)/%,$(notdir $(TEST_SRCS)))

# ----------------- Default target -----------------
all: $(TEST_BINS)

# ----------------- Build rule -----------------
$(TEST_DIR)/%: %.c
	@mkdir -p $(TEST_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# ----------------- Run all tests -----------------
run: all
	@for bin in $(TEST_BINS); do \
		echo "Running $$bin..."; \
		./$$bin; \
	done

# ----------------- Clean -----------------
clean:
	rm -rf $(TEST_DIR)

.PHONY: all run clean
