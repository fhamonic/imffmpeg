BUILD_DIR = build

.PHONY: all clean

all: $(BUILD_DIR)

$(BUILD_DIR):
	conan build . -of=${BUILD_DIR} -b=missing

clean:
	@rm -rf CMakeUserPresets.json
	@rm -rf $(BUILD_DIR)