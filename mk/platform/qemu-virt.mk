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
	drivers/block/virtio_blk.c \

# ---- 运行配置 --------------------------------------------------------------
QEMU_MACHINE := virt

# 【挂给 QEMU 的是磁盘镜像的一份**拷贝**, 不是镜像本身】
#
# 为什么这一点很重要: 内核会**写**这块盘 (文件系统、创建文件)。
# 如果直接把 build/.../disk.img 挂进去, 一次运行就会把它改脏 ——
# 而 make 不知道这件事 (它的时间戳没变), 于是**下一次运行看到的是
# 上一次留下的文件系统**:
#
#   第一次运行: 根目录只有 mkfs 放进去的 test_1..test_4
#   第二次运行: 根目录多出上次测试创建的文件
#
# 对实验验收来说这是致命的: README 里写的"期望输出"只对第一次运行成立,
# 而学生第一次跑出来的和第二次跑出来的不一样, 却找不到原因。
#
# 所以每次运行都从镜像复制一份出来跑。复制很小 (1 MB), 代价可以忽略。
# 用递归展开 (=) 而不是立即展开 (:=): DISKIMG 由 mk/build.mk 定义,
# 而 build.mk 在本文件之后才被 include。写成 := 的话这里会展开成空串,
# QEMU 就会收到 `-drive file=` 并直接报错。
QEMU_RUN_DISK = $(BUILD_DIR)/disk-run.img
QEMU_DRIVE_ARGS = -drive file=$(QEMU_RUN_DISK),if=none,format=raw,id=x0 \
                  -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
