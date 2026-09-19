# ============================================================================
# 目标配置: riscv64 / QEMU virt / OpenSBI 启动
# ----------------------------------------------------------------------------
# 这是一个 "profile" (配置档案): 一次性确定了 ISA、平台和启动路径三个维度。
# 学生只需要 `make CONFIG=riscv64-qemu-virt-sbi`，不需要自己拼参数，
# 也就不可能拼出诸如 "visionfive2 + direct M-mode" 这种非法组合。
#
# 本文件只负责"声明选择"，具体如何编译由 mk/arch/*.mk 和 mk/platform/*.mk 决定。
# ============================================================================

CONFIG_NAME := riscv64-qemu-virt-sbi

# ---- 三个正交的构建维度 ----------------------------------------------------
ARCH     := riscv64
   # CPU/ISA 语义:   寄存器、trap、页表、上下文切换
PLATFORM := qemu-virt
   # 机器/板级语义:  内存映射、设备地址、中断号、CPU 拓扑
BOOT     := sbi
   # 启动路径语义:   内核是如何被加载并接管的

# ---- 该配置的运行时参数 ----------------------------------------------------
# QEMU 的 -smp 必须与 platform.h 中的 PLAT_NCPU 保持一致，
# 否则内核会去启动不存在的 hart。这里集中定义，由 mk/platform/*.mk 消费。
QEMU_NCPU ?= 2
QEMU_MEM  ?= 128M

# QEMU virt 上 OpenSBI 固件: 使用 QEMU 自带的 default bios。
# -bios default 会把 OpenSBI 加载到 0x80000000，运行完跳转到 0x80200000 (S-mode)。
QEMU_BIOS ?= default

# 内核在内存中的加载/链接地址 (由 arch/riscv64/linker/kernel.ld 使用)
KERNEL_LOAD_ADDR := 0x80200000

# 该配置产生的构建产物目录名。不同配置的产物绝不互相覆盖，
# 所以切换平台不需要 clean。
BUILD_VARIANT := $(CONFIG_NAME)

# 平台目录/头文件路径提前确定, 因为 mk/common.mk 需要它来组成 -I 参数
PLATFORM_DIR := platform/$(PLATFORM)
PLATFORM_INCLUDES := -I$(PLATFORM_DIR)
