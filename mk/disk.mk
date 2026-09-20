DISK ?= $(BUILD)/disk.img
MKFS_FLAGS ?=
$(BUILD)/mkfs: tools/mkfs.c include/uapi/disk.h
	@mkdir -p $(@D)
	$(HOSTCC) -std=c11 -Wall -Wextra -Werror -Iinclude $< -o $@
.PHONY: disk
disk: $(BUILD)/mkfs user-programs
	@mkdir -p $(dir $(DISK))
	$(BUILD)/mkfs $(MKFS_FLAGS) $(DISK) $(USER_ELFS)
