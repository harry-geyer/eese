
BUILD_TESTS_DIR := $(BUILD_DIR)/tests

define TEST_OBJ_BUILD_RULE
$(1)_OBJECTS := $$(patsubst %.c,$$(BUILD_TESTS_DIR)/%.o,$(2))

$$($(1)_OBJECTS): $$(BUILD_TESTS_DIR)/%.o: %.c $$(BUILD_DIR)/.git.$$(GIT_COMMIT)
	mkdir -p $$(@D)
	echo $(addprefix $(BUILD_TESTS_DIR)/.,$(TESTS))
	# Using gcc instead of $(CC) as we want to use this object natively
	gcc -c -I$$(INCLUDE_DIR) $$< -o $$@

$$(BUILD_TESTS_DIR)/$(1).so: $$($(1)_OBJECTS)
	mkdir -p $$(@D)
	# Using gcc instead of $(CC) as we want to use this shared object natively
	gcc -shared -o $$@ $$^

$$(BUILD_TESTS_DIR)/.$(1): $$(BUILD_TESTS_DIR)/$(1).so
	mkdir -p $$(@D)
	touch $$@
endef

TESTS := ring_buf crc cobs

# Not using foreach as test might require multiple sources/different names
$(eval $(call TEST_OBJ_BUILD_RULE,ring_buf,$(SOURCE_DIR)/ring_buf.c))
$(eval $(call TEST_OBJ_BUILD_RULE,crc,$(SOURCE_DIR)/crc.c))
$(eval $(call TEST_OBJ_BUILD_RULE,cobs,libs/nanocobs/cobs.c))

$(BUILD_TESTS_DIR)/.complete: $(addprefix $(BUILD_TESTS_DIR)/.,$(TESTS))
	pytest --cov=python --rootdir=$(BUILD_TESTS_DIR) -v tests/

test: $(BUILD_TESTS_DIR)/.complete
	$(MAKE) -C libs/nanocobs
	./libs/nanocobs/build/cobs_unittests

coverage: test
	mkdir -p $(BUILD_DIR)/coverage
	python3-coverage html -d $(BUILD_DIR)/coverage
	sensible-browser $(BUILD_DIR)/coverage/index.html
