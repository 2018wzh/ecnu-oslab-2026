KERNEL_C := kernel/main.c kernel/lib/console.c kernel/lib/print.c kernel/lock/spinlock.c
KERNEL_C += kernel/lib/string.c kernel/mem/pmem.c kernel/mem/kvm.c
KERNEL_C += kernel/trap/trap.c kernel/trap/timer.c drivers/irqchip/plic.c
ARCH_S += arch/riscv64/trap_entry.S
KERNEL_C += kernel/proc/proc.c kernel/trap/user.c
ARCH_S += arch/riscv64/trampoline.S arch/riscv64/user_image.S
ARCH_S += arch/riscv64/switch.S
CPPFLAGS += -DUSER_IMAGE='"$(BUILD)/user/init.bin"'
$(BUILD)/user/init.elf: user/init.c user/syscall.c user/arch/$(ARCH)/entry.S user/sys.h user/arch/$(ARCH)/syscall_arch.h user/arch/$(ARCH)/user.ld include/uapi/syscall.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -Iuser/arch/$(ARCH) $(CFLAGS) -nostdlib -static -Wl,--no-relax,-T,user/arch/$(ARCH)/user.ld user/init.c user/syscall.c user/arch/$(ARCH)/entry.S -o $@
$(BUILD)/user/init.bin: $(BUILD)/user/init.elf
	$(OBJCOPY) -O binary $< $@
$(BUILD)/arch/riscv64/user_image.o: $(BUILD)/user/init.bin
KERNEL_C += kernel/mem/uvm.c kernel/mem/mmap.c kernel/syscall/syscall.c kernel/syscall/sysfunc.c kernel/syscall/memory.c
KERNEL_C += kernel/proc/lifecycle.c kernel/proc/schedule.c kernel/lock/sleeplock.c kernel/syscall/process.c
KERNEL_C += drivers/block/virtio_blk.c drivers/block/sd.c kernel/fs/block.c kernel/fs/buffer.c kernel/fs/bitmap.c kernel/fs/fs.c kernel/syscall/disk.c
KERNEL_C += kernel/fs/inode.c kernel/fs/dentry.c kernel/fs/lab8_examples.c
CPPFLAGS += -DLAB8_TEST=$(or $(LAB8_TEST),0)
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

# 例程选择是编译参数；切换 LAB8_TEST 后必须重新编译此对象。
.PHONY: lab8-example-config
$(BUILD)/kernel/fs/lab8_examples.o: lab8-example-config
