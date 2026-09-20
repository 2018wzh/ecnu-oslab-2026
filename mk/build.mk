KERNEL_C := kernel/main.c kernel/lib/console.c kernel/lib/print.c kernel/lock/spinlock.c
SOURCES := $(KERNEL_C) $(ARCH_C) $(PLATFORM_C)
OBJECTS := $(addprefix $(BUILD)/,$(SOURCES:.c=.o) $(ARCH_S:.S=.o))
-include $(OBJECTS:.o=.d)

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(BUILD)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.ld: arch/$(ARCH)/linker/kernel.lds.S platform/$(PLATFORM)/platform.h arch/$(ARCH)/include/asm/boot.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -E -P -x c -DLOAD=$(LOAD) $< -o $@
$(BUILD)/kernel.elf: $(OBJECTS) $(BUILD)/kernel.ld
	$(CC) $(CFLAGS) -nostdlib -static -Wl,--no-relax,-T,$(BUILD)/kernel.ld $(OBJECTS) -lgcc -o $@
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
.PHONY: build
build: $(BUILD)/kernel.elf
