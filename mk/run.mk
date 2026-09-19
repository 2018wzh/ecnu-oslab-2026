# ============================================================================
# mk/run.mk -- 运行与调试
# ----------------------------------------------------------------------------
# QEMU 可以直接运行; 开发板必须"交付镜像 + 人工烧写", 所以 run 目标
# 对开发板只打印操作步骤, 不假装自己能烧卡。
#
# 这种"明确区分能自动化的和不能自动化的"也是工程习惯:
# 一个悄悄执行 sudo dd 的 run 目标非常危险 —— 写错设备名会清掉学生的硬盘。
# ============================================================================

# QEMU 的二进制名跟着架构走: qemu-system-riscv64 / qemu-system-aarch64 …
# 想用别的模拟器时在环境里覆盖它: QEMU=/path/to/qemu-system-x86_64
QEMU ?= qemu-system-$(QEMU_ARCH)
GDBPORT := $(shell expr `id -u` % 5000 + 25000)

# ---- QEMU 运行参数 ---------------------------------------------------------
QEMU_OPTS := -machine $(QEMU_MACHINE)
QEMU_OPTS += -nographic
QEMU_OPTS += -m $(QEMU_MEM)
QEMU_OPTS += -smp $(QEMU_NCPU)
QEMU_OPTS += -bios $(QEMU_BIOS)
QEMU_OPTS += -kernel $(KERNEL_ELF)
ifneq ($(QEMU_DRIVE_ARGS),)
QEMU_OPTS += $(QEMU_DRIVE_ARGS)
endif

# ---- 运行 ------------------------------------------------------------------
.PHONY: run
run: build
ifeq ($(PLATFORM),qemu-virt)
	@# 每次都从干净的镜像复制一份来跑 —— 见 mk/platform/qemu-virt.mk
	@# 里 QEMU_RUN_DISK 的说明 (内核会写这块盘, 直接挂会污染镜像)。
	@if [ -f "$(DISKIMG)" ]; then cp -f "$(DISKIMG)" "$(QEMU_RUN_DISK)"; fi
	@echo "启动 QEMU (退出: Ctrl-A 然后 X)"
	$(QEMU) $(QEMU_OPTS)
else
	@echo "=========================================================="
	@echo "  $(PLATFORM) 是真实开发板, 无法由 make 直接运行。"
	@echo "  请按以下步骤部署:"
	@echo ""
	@echo "  1) 生成镜像 (已自动完成): $(FIT_IMAGE)"
	@echo "  2) 把 $(FIT_IMAGE) 拷贝到 SD 卡第一个 FAT 分区"
	@echo "  3) 串口连接开发板 (115200 8N1), 上电进入 U-Boot"
	@echo "  4) 在 U-Boot 中执行:"
	@echo "       $(UBOOT_BOOT_CMD)"
	@echo ""
	@echo "  详细说明见 docs/board-deploy.md"
	@echo "=========================================================="
endif

# ---- 调试 ------------------------------------------------------------------
.PHONY: debug
debug: build .gdbinit
ifeq ($(PLATFORM),qemu-virt)
	@if [ -f "$(DISKIMG)" ]; then cp -f "$(DISKIMG)" "$(QEMU_RUN_DISK)"; fi
	@echo "QEMU 等待 gdb 连接 (端口 $(GDBPORT))..."
	@echo "另开一个终端执行: $(TOOLPREFIX)gdb $(KERNEL_ELF)"
	$(QEMU) $(QEMU_OPTS) -S -gdb tcp::$(GDBPORT)
else
	@echo "开发板调试需要 JTAG 调试器, 本课程不要求。"
	@echo "QEMU 上的调试请使用: make CONFIG=riscv64-qemu-virt-sbi debug"
endif

.gdbinit: .gdbinit.tmpl
	@sed "s/:1234/:$(GDBPORT)/" < $< > $@

# ---- 反汇编 (排查启动问题时非常有用) ---------------------------------------
.PHONY: disasm
disasm: $(KERNEL_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(BUILD_DIR)/kernel.asm
	@echo "反汇编已写入 $(BUILD_DIR)/kernel.asm"
	@echo "提示: 排查启动问题时, 先确认入口地址与链接地址一致:"
	@$(OBJDUMP) -h $(KERNEL_ELF) | head -20

.PHONY: gdbinit-clean
gdbinit-clean:
	rm -f .gdbinit
