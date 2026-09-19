# ============================================================================
# ECNU OSLab 2026 -- 顶层 Makefile
# ----------------------------------------------------------------------------
# 设计原则 (借鉴 Linux Kbuild 的边界, 但不照搬其复杂度):
#   1. 顶层 Makefile 是 generic 的: 它只知道"有 arch / platform / boot 三个维度",
#      不知道 RISC-V 也不知道 QEMU。
#   2. 具体某个架构编译哪些文件 -> mk/arch/$(ARCH).mk
#      具体某台机器包含哪些资源 -> mk/platform/$(PLATFORM).mk
#      具体怎么被加载        -> mk/boot/$(BOOT).mk
#   3. 不使用 $(wildcard arch/*/*.c) 这类收集方式:
#      一旦存在第二个架构, wildcard 会把两边的目标文件都链进来。
#
# 用法:
#   make CONFIG=riscv64-qemu-virt-sbi       # 构建 QEMU 版本
#   make CONFIG=riscv64-qemu-virt-sbi run   # 构建并运行
#   make CONFIG=riscv64-visionfive2-uboot   # 构建开发板 FIT 镜像
#   make list                               # 列出所有可用配置
# ============================================================================

# ---- 1. 选择配置档案 -------------------------------------------------------
# 默认使用 QEMU 配置, 方便学生一上手就能跑起来。
CONFIG ?= riscv64-qemu-virt-sbi
CONFIG_FILE := configs/$(CONFIG).mk

ifeq ($(wildcard $(CONFIG_FILE)),)
  $(error 未知配置 '$(CONFIG)'。可用配置见 `make list`)
endif

include $(CONFIG_FILE)

# ---- 2. 载入三个维度的构建描述 ---------------------------------------------
include mk/common.mk
include mk/arch/$(ARCH).mk
include mk/platform/$(PLATFORM).mk
include mk/boot/$(BOOT).mk
include mk/build.mk
include mk/image.mk
include mk/run.mk

# ---- 3. 校验组合合法性 -----------------------------------------------------
# profile 机制的一个好处: 可以把"非法组合"挡在编译之前,
# 而不是等学生在真机上跑飞了才发现。
include mk/validate.mk

# ---- 4. 便捷目标 -----------------------------------------------------------
.PHONY: all
all: build

.PHONY: help
help:
	@echo "ECNU OSLab 2026 -- 可用目标"
	@echo ""
	@echo "  make CONFIG=<配置> build     构建内核与用户程序"
	@echo "  make CONFIG=<配置> run       构建并运行 (QEMU) 或提示烧写步骤 (开发板)"
	@echo "  make CONFIG=<配置> debug     以调试模式启动 QEMU (等待 gdb)"
	@echo "  make CONFIG=<配置> image     生成可交付镜像 (QEMU: disk.img / VF2: kernel.itb)"
	@echo "  make CONFIG=<配置> clean     清理该配置的构建产物"
	@echo "  make list                    列出全部可用配置"
	@echo "  make info                    打印当前配置的详情"
	@echo ""
	@echo "  当前默认配置: $(CONFIG)"

.PHONY: list
list:
	@echo "可用配置 (configs/*.mk):"
	@echo ""
	@# 从每个配置文件里提取三个维度的取值。
	@# 【为什么用 grep 而不是 . (source)】
	@# 直接 source 这些 .mk 文件会执行其中的赋值与 shell 调用,
	@# 而且是在 make 的 recipe 子 shell 里 —— 变量作用域容易出问题
	@# (之前这里就出现过 ARCH/PLATFORM/BOOT 打印为空的情况)。
	@# 用 grep 只读取我们需要的三行, 简单且不会互相干扰。
	@for f in configs/*.mk; do \
		n=$$(basename $$f .mk); \
		a=$$(grep -E '^ARCH[[:space:]]*:=' $$f | head -1 | sed 's/.*:=[[:space:]]*//'); \
		p=$$(grep -E '^PLATFORM[[:space:]]*:=' $$f | head -1 | sed 's/.*:=[[:space:]]*//'); \
		b=$$(grep -E '^BOOT[[:space:]]*:=' $$f | head -1 | sed 's/.*:=[[:space:]]*//'); \
		printf "  %-30s ARCH=%-8s PLATFORM=%-12s BOOT=%s\n" "$$n" "$$a" "$$p" "$$b"; \
	done
	@echo ""
	@echo "用法: make CONFIG=<配置名> [build|run|image|debug|info|clean]"
	@echo "例如: make CONFIG=riscv64-qemu-virt-sbi run"

.PHONY: info
info:
	@echo "================ 当前配置 ================"
	@echo "CONFIG        : $(CONFIG)"
	@echo "ARCH          : $(ARCH)   (ISA 语义)"
	@echo "PLATFORM      : $(PLATFORM)   (机器语义)"
	@echo "BOOT          : $(BOOT)   (启动路径语义)"
	@echo "工具链前缀    : $(TOOLPREFIX)"
	@echo "内核加载地址  : $(KERNEL_LOAD_ADDR)"
	@echo "链接脚本模板  : $(LDSCRIPT_TEMPLATE)"
	@echo "构建产物目录  : $(BUILD_DIR)"
	@echo "平台头文件    : $(PLATFORM_DIR)/platform.h"
	@echo "内核源文件    :"
	@for f in $(KERNEL_ALL_CSRCS) $(KERNEL_ALL_SSRCS); do echo "    $$f"; done
	@echo "=========================================="
