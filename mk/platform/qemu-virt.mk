# ============================================================================
# platform/qemu-virt-riscv64.mk -- QEMU virt 机器的构建描述
# ----------------------------------------------------------------------------
# platform 层负责回答: "这台机器的内存和设备在哪里?"
# 具体地址写在 platform/qemu-virt-riscv64/platform.h 里,
# 这里只负责声明该平台包含哪些 C 文件、以及如何运行它。
# ============================================================================

PLATFORM_DIR := platform/qemu-virt-riscv64

# ---- 平台源文件 ------------------------------------------------------------
# platform.c 负责板级初始化流程 (把内存布局、设备实例交给 generic kernel)
PLATFORM_CSRCS := \
	$(PLATFORM_DIR)/platform.c

# ---- 平台头文件搜索路径 ----------------------------------------------------
# 关键设计: 用 -I 指向"某一个"平台目录, 于是所有驱动都能写
#     #include <platform.h>
# 编译 QEMU 配置时解析到 qemu-virt 的地址, 编译 VF2 配置时解析到 VF2 的地址。
# 这就是"用构建系统选择平台"而不是"在代码里 #ifdef PLATFORM_XXX"。
PLATFORM_INCLUDES := -I$(PLATFORM_DIR)

# ---- 该平台需要的驱动 ------------------------------------------------------
# QEMU virt 提供 PLIC + CLINT + 16550 UART + VirtIO-MMIO 磁盘
PLATFORM_DRIVERS := \
	drivers/serial/uart16550.c \
	drivers/irqchip/plic.c \

# ---- 运行配置 ----------------------------------------------------
QEMU_MACHINE := virt
