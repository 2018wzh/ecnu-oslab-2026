# ============================================================================
# 目标配置: riscv64 / VisionFive2 (昉·星光2) / U-Boot 启动
# ----------------------------------------------------------------------------
# 与 riscv64-qemu-virt-sbi 相比:
#   ARCH     相同  -> riscv64    (ISA 不变)
#   PLATFORM 不同  -> visionfive2 (板级资源变了)
#   BOOT     不同  -> uboot      (启动交接方式变了)
#
# 这个组合正是检验抽象是否成功的关键:
#   换到本配置时, kernel/ 与 arch/riscv64/ 应当"几乎"零修改,
#   改动应当集中在 platform/visionfive2/ 与 drivers/。
# 如果发现必须去改 kernel/mm/ 或 kernel/proc/, 说明边界划分错了。
# ============================================================================

CONFIG_NAME := riscv64-visionfive2-uboot

# ---- 三个正交的构建维度 ----------------------------------------------------
ARCH     := riscv64
PLATFORM := visionfive2
BOOT     := uboot

# ---- 该配置的运行时参数 ----------------------------------------------------
# VisionFive2 (JH7110) 有 4 个 U74 应用核 + 1 个 S7 监控核。
# U-Boot 自己运行在 S7 上, 把 U74 交给内核。
# 详见 platform/visionfive2/platform.h 中的 PLAT_NCPU 与 PLAT_BOOT_HART 注释。
PLAT_NCPU ?= 4

# 内核链接地址: OpenSBI 在 0x40000000, 运行完跳转到 0x40200000 (S-mode)。
KERNEL_LOAD_ADDR := 0x40200000

# ---- U-Boot 加载方式 -------------------------------------------------------
# 使用 FIT (Flattened Image Tree) 镜像 + booti。
# 为什么不用裸 dd + go:
#   1. 裸镜像没有头部信息, U-Boot 无法校验架构/加载地址, 容易静默跑飞
#   2. 固定 dd 偏移依赖 SD 卡具体容量和分区表, 换一张卡就失效
#   3. FIT 是 U-Boot 的标准镜像格式, 也是真实产品里部署内核的方式
UBOOT_IMAGE_FORMAT := fit
UBOOT_LOAD_ADDR    := $(KERNEL_LOAD_ADDR)
UBOOT_ENTRY_ADDR   := $(KERNEL_LOAD_ADDR)

BUILD_VARIANT := $(CONFIG_NAME)

# 平台目录/头文件路径提前确定, 因为 mk/common.mk 需要它来组成 -I 参数
PLATFORM_DIR := platform/$(PLATFORM)
PLATFORM_INCLUDES := -I$(PLATFORM_DIR)
