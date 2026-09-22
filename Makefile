# Builds and runs every test/*_test.c file (helpers in subfolders, e.g.
# test/unity/, are ignored).
# Each test file must have its gcc compile command as a comment on the
# first line, in the form: // <any label>: gcc ... -o build/<name>

BUILD_DIR := build
TEST_SRCS := $(wildcard test/*_test.c)

.PHONY: all test build clean

all: test

build: $(TEST_SRCS)
	@mkdir -p $(BUILD_DIR)
	@for src in $(TEST_SRCS); do \
		cmd=$$(sed -n '1s#^// *[^:]*: *##p' $$src); \
		if [ -z "$$cmd" ]; then \
			echo "!! No compile command found on line 1 of $$src (expected '// <label>: gcc ...')"; \
			exit 1; \
		fi; \
		echo "=== Building $$src ==="; \
		echo "$$cmd"; \
		eval $$cmd || exit 1; \
	done

test: build
	@status=0; \
	for src in $(TEST_SRCS); do \
		cmd=$$(sed -n '1s#^// *[^:]*: *##p' $$src); \
		bin=$$(echo "$$cmd" | sed -n 's/.*-o  *\([^ ]*\).*/\1/p'); \
		if [ -z "$$bin" ]; then \
			echo "!! Could not find -o <binary> in compile command for $$src"; \
			status=1; \
			continue; \
		fi; \
		echo "=== Running $$bin ==="; \
		./$$bin; \
		rc=$$?; \
		if [ $$rc -ne 0 ]; then status=1; fi; \
		echo; \
	done; \
	exit $$status

clean:
	rm -rf $(BUILD_DIR)
