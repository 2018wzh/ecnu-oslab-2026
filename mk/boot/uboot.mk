# ============================================================================
# mk/boot/uboot.mk -- 通过 U-Boot 加载的启动路径
# ----------------------------------------------------------------------------
# U-Boot 内部运行在 OpenSBI 之上, 所以它交给内核时的状态与纯 OpenSBI
# **完全相同** (a0=hartid, a1=dtb, satp=0) —— 因此入口汇编、早期控制台、
# 从核启动、时钟设置这四样都可以与 mk/boot/sbi.mk 共用。
#
# 真正不同的只有"交付格式": 内核必须打成 U-Boot 认识的 FIT 镜像 (.itb),
# 加载地址由 FIT 头声明。那是**镜像/部署**问题, 不属于内核逻辑。
#
# 这正是判断边界划得对不对的地方: 接 U-Boot 时如果要去改 kernel/, 说明
# 抽象漏了。本文件与 sbi.mk 的差异只有下面 FIT 那两行。
# ============================================================================

BOOT_NAME := U-Boot

BOOT_SSRCS := $(ARCH_BOOT_DIR)/entry.S

BOOT_CSRCS := \
	$(ARCH_BOOT_DIR)/sbi/early_console.c \
	$(ARCH_BOOT_DIR)/sbi/smp_hsm.c \
	$(ARCH_BOOT_DIR)/sbi/time.c

BOOT_LD_DEFS :=

BOOT_CFLAGS := -DCONFIG_BOOT_UBOOT=1 -DCONFIG_BOOT_HAS_FIRMWARE=1 -DBOOT_NAME='"U-Boot"'

# ---- 镜像格式 --------------------------------------------------------------
# FIT 镜像 (.itb) 用一个 .its 源文件描述: 内核镜像、加载地址、入口地址、架构。
FIT_REQUIRED := 1

# ---- U-Boot 侧加载命令 (供 README 与学生手册引用) --------------------------
#   => mmc dev 1
#   => fatload mmc 1:1 ${kernel_addr_r} kernel.itb
#   => bootm ${kernel_addr_r}
#
# 为什么用 bootm 而不是裸 go: go 只是无条件跳转, 不校验镜像类型/架构/校验和;
# bootm 会解析 FIT 头, 校验架构与校验和, 并按声明的地址正确放置镜像。
UBOOT_BOOT_CMD = fatload mmc 1:1 \$${kernel_addr_r} kernel.itb; bootm \$${kernel_addr_r}
