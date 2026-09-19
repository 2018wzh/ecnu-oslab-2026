# ============================================================================
# mk/boot/sbi.mk -- 通过 OpenSBI (纯 SBI 固件) 接管的启动路径
# ----------------------------------------------------------------------------
# 启动路径是一个**独立维度**: 它决定"内核是怎么被交给 CPU 的"。
#
#   QEMU virt + OpenSBI      -> 本文件
#   VisionFive2 + U-Boot     -> mk/boot/uboot.mk (同样是 S-mode 交接)
#   裸机直启 (无固件)         -> mk/boot/raw.mk  (M-mode 过渡由内核自己做)
#
# 【这个维度要提供四样东西, 一个都不能少】
#
#   1. BOOT_SSRCS  入口汇编
#      内核从这里开始执行。不同协议的进入状态完全不同 (M-mode? S-mode?
#      a0 是什么?), 所以它属于协议。
#
#   2. BOOT_CSRCS  早期控制台 / 从核启动 / 时钟设置
#      "最早的输出往哪写"取决于有没有固件 (SBI 控制台 vs 直接写 UART);
#      "怎么启动其他核"取决于有没有 HSM 扩展; "怎么设下一次时钟中断"
#      取决于有没有 TIME 扩展或 Sstc。这三件事全部由协议决定。
#
#   3. BOOT_LD_DEFS  链接脚本参数
#      协议要求的额外段 (multiboot2 头、limine requests…) 在这里追加,
#      通用模板见 arch/<arch>/linker/kernel.ld.in。
#
#   4. BOOT_CFLAGS   编译期宏
#
# 【统一约定: RISC-V S-mode 启动 ABI】(与 Linux 相同)
#   a0 = hartid, a1 = dtb 物理地址, satp = 0, sp = 未定义, SIE = 0
# ============================================================================

BOOT_NAME := SBI

# 入口: S-mode 直接交接 (与 U-Boot 相同 —— 两者都跑在 OpenSBI 之上)
BOOT_SSRCS := $(ARCH_BOOT_DIR)/entry.S

# 协议提供的实现: 早期控制台 / 从核启动 / 时钟
BOOT_CSRCS := \
	$(ARCH_BOOT_DIR)/sbi/early_console.c \
	$(ARCH_BOOT_DIR)/sbi/smp_hsm.c \
	$(ARCH_BOOT_DIR)/sbi/time.c

# 链接脚本参数: 本协议不需要额外段
BOOT_LD_DEFS :=

BOOT_CFLAGS := -DCONFIG_BOOT_SBI=1 -DCONFIG_BOOT_HAS_FIRMWARE=1 -DBOOT_NAME='"SBI"'
