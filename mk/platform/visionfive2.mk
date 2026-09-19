# ============================================================================
# platform/visionfive2.mk -- VisionFive2 (昉·星光2, JH7110) 的构建描述
# ============================================================================

PLATFORM_DIR := platform/visionfive2

PLATFORM_CSRCS := \
	$(PLATFORM_DIR)/platform.c

PLATFORM_INCLUDES := -I$(PLATFORM_DIR)

# ---- 该平台需要的驱动 ------------------------------------------------------
# 与 QEMU 的差异正是"驱动复用"的检验:
#   - 同样是 SiFive PLIC  -> 复用 drivers/irqchip/plic.c (只换地址)
#   - 同样是 16550 UART   -> 复用 drivers/serial/uart16550.c
#   - 不再是 VirtIO 磁盘  -> 换成 drivers/block/sdhci.c (DW MSHC SD 控制器)
#   - 定时器               -> 与 QEMU 相同, 但**实现由启动协议给**:
#                             有固件走 SBI/ TIME, 裸机走 Sstc
#
# 定时器**不再分平台**: 读时间用 rdtime CSR、设置中断用 SBI, 这条路
# 在两个平台上完全一样。原先分成 riscv_timer.c / sbi_timer.c 两份,
# 逐行比对后发现代码完全相同 —— 那种"必须永远保持同步的重复"迟早会
# 不一致, 且症状会出现在没改动的那一侧。见该文件的说明。
#
# 也就是说 4 个驱动里有 3 个完全复用, 学生应能解释为什么它们能复用,
# 以及第 4 个 (SD 卡) 为什么不能 —— 它换的是**设备协议**, 不是地址。
PLATFORM_DRIVERS := \
	drivers/serial/uart16550.c \
	drivers/irqchip/plic.c \

# ---- 运行配置 --------------------------------------------------------------
# VisionFive2 通过 U-Boot 加载, 没有 QEMU 参数。
# 产物为 FIT 镜像 (见 mk/image.mk), 由 U-Boot 的 booti 或 bootm 加载。
PLATFORM_DEPLOY := fit
