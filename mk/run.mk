QEMU := qemu-system-riscv64 -machine virt -bios default -m 128M -smp 2 -nographic
.PHONY: run debug
ifeq ($(PLATFORM),qemu-virt)
run: build
	$(QEMU) -kernel $(BUILD)/kernel.elf -global virtio-mmio.force-legacy=false -drive file=$(DISK),if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
debug: build
	$(QEMU) -kernel $(BUILD)/kernel.elf -S -gdb tcp::1234 -global virtio-mmio.force-legacy=false -drive file=$(DISK),if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
else
run debug:
	@echo '使用 image 生成 kernel.itb，再通过 U-Boot bootm 启动。'
endif
