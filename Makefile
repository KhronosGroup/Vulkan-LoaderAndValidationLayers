# Linux makefile for
# https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers
# mikew@lunarg.com

RELEASE_DIR  = build
DEBUG_DIR    = dbuild
EXTERNAL_DIR = external

TARGET_NAME = vk_layer_validation_tests
RELEASE_TARGET = $(RELEASE_DIR)/tests/$(TARGET_NAME)
DEBUG_TARGET = $(DEBUG_DIR)/tests/$(TARGET_NAME)

.DELETE_ON_ERROR: $(RELEASE_TARGET) $(DEBUG_TARGET)


.PHONY: all
all: $(RELEASE_TARGET) $(DEBUG_TARGET)

$(EXTERNAL_DIR):
	./update_external_sources.sh

$(RELEASE_DIR): $(EXTERNAL_DIR)
	cmake -H. -B$@ -DCMAKE_BUILD_TYPE=Release

$(DEBUG_DIR): $(EXTERNAL_DIR)
	cmake -H. -B$@ -DCMAKE_BUILD_TYPE=Debug

$(RELEASE_TARGET): $(RELEASE_DIR)
	$(MAKE) -C $(RELEASE_DIR)

$(DEBUG_TARGET): $(DEBUG_DIR)
	$(MAKE) -C $(DEBUG_DIR)


.PHONY: test_release
test_release: $(RELEASE_TARGET)
	cd $(RELEASE_DIR)/tests && VK_LAYER_PATH=$(PWD)/$(RELEASE_DIR)/layers ./$(TARGET_NAME)

.PHONY: test_debug
test_debug: $(DEBUG_TARGET)
	cd $(DEBUG_DIR)/tests && VK_LAYER_PATH=$(PWD)/$(DEBUG_DIR)/layers ./$(TARGET_NAME)

.PHONY: t test
t test: test_release test_debug

.PHONY: clean
clean:
	-rm -f $(RELEASE_TARGET)
	-rm -f $(DEBUG_TARGET)

.PHONY: clobber
clobber: clean
	-rm -rf $(RELEASE_DIR)
	-rm -rf $(DEBUG_DIR)

.PHONY: nuke
nuke: clobber
	-rm -rf $(EXTERNAL_DIR)

# vim: set sw=4 ts=8 noet ic ai:
