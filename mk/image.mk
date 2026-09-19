# ============================================================================
# mk/image.mk -- 可交付镜像的生成
# ----------------------------------------------------------------------------
# QEMU:   disk.img 就是最终产物, 由 -drive 挂载
# VF2:    kernel.itb (FIT 镜像) 是最终产物, 由 U-Boot 的 bootm 加载
#
# 关于 FIT (Flattened Image Tree):
#   FIT 是 U-Boot 的标准镜像格式。它在内核二进制外面包一层 FDT 描述,
#   声明: 架构、类型、加载地址、入口地址、校验方式。
#   U-Boot 加载时会解析这份描述, 因此可以:
#     - 拒绝与自身架构不匹配的镜像 (避免静默跑飞)
#     - 按声明的地址放置镜像, 不依赖人工计算的 dd 偏移
#     - 校验数据完整性
#
#   2025 版本使用 `dd seek=20GB` + `go 0x40200000`:
#     偏移量依赖具体 SD 卡容量, 换一张卡就失效
#     裸跳转不做任何校验, 地址写错就静默跑飞
#
#   镜像由 tools/image/fitgen.c 生成 (宿主工具)。
#   不依赖 u-boot-tools, 因为 FIT 结构本身很简单,
#   自己实现一遍能让学生真正看懂 U-Boot 是如何认识内核的。
# ============================================================================

ifeq ($(FIT_REQUIRED),1)

FITGEN    := $(BUILD_DIR)/host/fitgen
FIT_IMAGE := $(BUILD_DIR)/kernel.itb

$(FITGEN): tools/image/fitgen.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $<
	@echo "  [host-cc] fitgen"

$(FIT_IMAGE): $(KERNEL_BIN) $(FITGEN)
	@$(FITGEN) $(KERNEL_BIN) $@ $(UBOOT_LOAD_ADDR) $(UBOOT_ENTRY_ADDR) \
		"ECNU OSLab 2026 ($(CONFIG))"

.PHONY: image
image: build $(FIT_IMAGE)
	@echo ""
	@echo "===== 开发板交付物已生成 ====="
	@echo "  FIT 镜像  : $(FIT_IMAGE)"
	@echo "  磁盘镜像  : $(DISKIMG)"
	@echo ""
	@echo "  部署步骤 (详见 docs/board-deploy.md):"
	@echo "   1) 把 SD 卡第一个分区格式化为 FAT32"
	@echo "   2) 拷贝 $(FIT_IMAGE) 与 $(DISKIMG) 到该分区"
	@echo "   3) 串口连接开发板 (115200 8N1), 上电"
	@echo "   4) 在 U-Boot 提示符下执行:"
	@echo "        $(UBOOT_BOOT_CMD)"
	@echo ""

else

.PHONY: image
image: build
	@echo ""
	@echo "===== QEMU 运行镜像已就绪 ====="
	@echo "  内核     : $(KERNEL_ELF)"
	@echo "  磁盘镜像 : $(DISKIMG)"
	@echo "  运行     : make CONFIG=$(CONFIG) run"

endif
