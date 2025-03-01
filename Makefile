.PHONY: all build test clean format coverage \
	example-event-loop example-http example-readme example-simple-coro

BUILD_DIR = build

all: test

build:
	if [ -d $(BUILD_DIR) ]; then \
		meson setup $(BUILD_DIR) --reconfigure; \
	else \
		meson setup $(BUILD_DIR) -Db_coverage=true; \
	fi
	ninja -C $(BUILD_DIR)

test: build
	cd $(BUILD_DIR) && meson test --print-errorlogs --timeout 0

clean:
	rm -rf $(BUILD_DIR)

format:
	git ls-tree -r HEAD --name-only | grep -E "cpp|hpp" | xargs -I {} sh -c 'clang-tidy {} -p ./build --fix-errors'
	git ls-tree -r HEAD --name-only | grep -E "cpp|hpp" | xargs -I {} sh -c 'clang-format -i {}'

list:
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'

coverage:
	cd $(BUILD_DIR) && ninja coverage

example-event-loop: build
	$(BUILD_DIR) && ./examples/event-loop

example-http: build
	$(BUILD_DIR) && ./examples/http

example-readme: build
	$(BUILD_DIR) && ./examples/readme

example-simple-coro: build
	$(BUILD_DIR) && ./examples/simple-coro
