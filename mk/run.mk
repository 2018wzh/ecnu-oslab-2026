QEMU := qemu-system-riscv64 -machine virt -bios default -m 128M -smp 2 -nographic
.PHONY: run debug
ifeq ($(PLATFORM),qemu-virt)
run: build
	$(QEMU) -kernel $(BUILD)/kernel.elf
debug: build
	$(QEMU) -kernel $(BUILD)/kernel.elf -S -gdb tcp::1234
else
run debug:
	@echo '使用 image 生成 kernel.itb，再通过 U-Boot bootm 启动。'
endif
