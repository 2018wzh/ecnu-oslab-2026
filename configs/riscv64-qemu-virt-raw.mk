# ============================================================================
# 目标配置: riscv64 / QEMU virt / **裸机直启** (无固件)
# ----------------------------------------------------------------------------
# 与前两个配置的唯一区别是 BOOT 维度: 这里没有 OpenSBI / U-Boot,
# QEMU 用 `-bios none` 把内核加载到链接地址并直接在 M-mode 跳进来。
#
# 它的价值是**证明启动路径是可插拔的**:
#   arch     = riscv64     (不变)
#   platform = qemu-virt   (不变 —— 还是同一台虚拟板子)
#   boot     = raw         (变 —— 从"固件交接到 S-mode"变成"自己从 M-mode 降级")
# kernel/ 下一个文件都没有为它改过。
# ============================================================================

CONFIG_NAME := riscv64-qemu-virt-raw

ARCH     := riscv64
PLATFORM := qemu-virt
BOOT     := raw

# 没有固件, 内核被直接加载到 DRAM 起点
KERNEL_LOAD_ADDR := 0x80000000

# -bios none: 不加载任何固件, 内核就是"固件"
QEMU_BIOS ?= none
QEMU_NCPU ?= 2
QEMU_MEM  ?= 128M

BUILD_VARIANT := $(CONFIG_NAME)

PLATFORM_DIR := platform/$(PLATFORM)
PLATFORM_INCLUDES := -I$(PLATFORM_DIR)
