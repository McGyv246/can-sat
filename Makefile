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
	@status=0; total_n=0; total_f=0; total_i=0; \
	summary_file=$(BUILD_DIR)/.test_summary; \
	: > $$summary_file; \
	for src in $(TEST_SRCS); do \
		cmd=$$(sed -n '1s#^// *[^:]*: *##p' $$src); \
		bin=$$(echo "$$cmd" | sed -n 's/.*-o  *\([^ ]*\).*/\1/p'); \
		if [ -z "$$bin" ]; then \
			echo "!! Could not find -o <binary> in compile command for $$src"; \
			status=1; \
			printf '  %-28s %s\n' "$$src" 'no binary found' >> $$summary_file; \
			continue; \
		fi; \
		echo "=== Running $$bin ==="; \
		out=$$(./$$bin; echo "RC:$$?"); \
		echo "$$out" | sed '$$d'; \
		rc=$$(echo "$$out" | sed -n 's/^RC://p'); \
		if [ "$$rc" -ne 0 ]; then status=1; fi; \
		line=$$(echo "$$out" | grep -oE '[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored' | tail -1); \
		if [ -z "$$line" ]; then \
			printf '  %-28s %s\n' "$$src" 'no Unity summary found' >> $$summary_file; \
		else \
			n=$$(echo "$$line" | awk '{print $$1}'); f=$$(echo "$$line" | awk '{print $$3}'); i=$$(echo "$$line" | awk '{print $$5}'); \
			p=$$((n - f - i)); \
			total_n=$$((total_n + n)); total_f=$$((total_f + f)); total_i=$$((total_i + i)); \
			printf '  %-28s %3d tests  %3d passed  %3d failed  %3d ignored\n' "$$src" "$$n" "$$p" "$$f" "$$i" >> $$summary_file; \
		fi; \
		echo; \
	done; \
	echo "==================== Test Summary ===================="; \
	cat $$summary_file; \
	rm -f $$summary_file; \
	echo "---------------------------------------------------------"; \
	total_p=$$((total_n - total_f - total_i)); \
	printf 'TOTAL: %d file(s), %d tests, %d passed, %d failed, %d ignored\n' "$(words $(TEST_SRCS))" "$$total_n" "$$total_p" "$$total_f" "$$total_i"; \
	exit $$status

clean:
	rm -rf $(BUILD_DIR)
