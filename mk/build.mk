# ============================================================================
# mk/build.mk -- 目标文件图 (object graph) 的组装与编译规则
# ----------------------------------------------------------------------------
# 核心思想: 内核的所有源文件由三个来源相加而成, 每个来源由各自的维度决定。
#
#   generic kernel  (kernel/)   -- OS 语义, 与架构/平台无关
#   arch            (arch/...)  -- CPU 语义
#   platform+drivers            -- 机器与设备语义
#
# 三者相加的结果就是最终链接进内核的全部代码。
# `make info` 会把这三个列表打印出来, 学生应当能够解释
# "为什么这个文件被选中了"。
# ============================================================================

# ---- 1. 构建产物目录 -------------------------------------------------------
# 目录名包含配置名, 因此:
#   build/riscv64-qemu-virt-sbi/...       和
#   build/riscv64-visionfive2-uboot/...
# 互不干扰。切换平台不需要 clean —— 分析报告里特别强调
# "换平台前必须先 clean 不应成为正常使用前提"。
BUILD_DIR := build/$(BUILD_VARIANT)
TARGET    := $(BUILD_DIR)

# ---- 2. generic kernel 源文件 (显式列出) -----------------------------------
# 注意 kernel/ 里没有任何一个文件知道自己是跑在 QEMU 还是 VisionFive2 上,
# 也不知道 CPU 是 RISC-V 还是别的。它们只调用 include/kernel/ 里的接口。
KERNEL_CORE_CSRCS := \
	kernel/init/main.c \
	kernel/lib/print.c \
	kernel/lib/string.c \
	kernel/sync/spinlock.c \
	kernel/lib/console.c \
	kernel/mm/pmem.c \
	kernel/mm/vm.c \
	kernel/mm/kvmmap.c \

# ---- 3. 汇总所有源文件 -----------------------------------------------------
KERNEL_ALL_CSRCS := $(KERNEL_CORE_CSRCS) $(ARCH_CSRCS) $(PLATFORM_CSRCS) $(PLATFORM_DRIVERS) $(BOOT_CSRCS)
KERNEL_ALL_SSRCS := $(ARCH_SSRCS) $(BOOT_SSRCS)

# 目标文件: 源码路径 -> build/<variant>/<原路径>.o
# 保留目录结构, 便于学生对照源码定位目标文件。
KERNEL_OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_ALL_CSRCS))
KERNEL_OBJS += $(patsubst %.S,$(BUILD_DIR)/%.o,$(KERNEL_ALL_SSRCS))

# ---- 4. 内核 ELF -----------------------------------------------------------
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# ---- 由模板生成链接脚本 ------------------------------------------------------
# 【为什么链接脚本要"生成"而不是直接写一份】
# 一份脚本里同时有"架构事实"(架构名、页大小)、"平台事实"(加载基址) 和
# "启动协议事实"(协议自己需要的段)。写死任何一项, 换其中一个维度时都得
# 去改那份脚本 —— 而脚本是可执行规格, 改错一个字符就是内核跑飞。
#
# 现在: 模板 (arch 提供) + 参数 (三个维度各自提供) -> 生成的 .ld。
# 想加一个启动协议, 只需要一个新的 mk/boot/<proto>.mk, 内核与构建规则不动。
KERNEL_LD := $(BUILD_DIR)/kernel.ld

LDSCRIPT_DEFS ?=
# 入口符号: 启动协议可以覆盖 (见 mk/boot/raw.mk)
KERNEL_ENTRY_SYMBOL = $(if $(BOOT_ENTRY_SYMBOL),$(BOOT_ENTRY_SYMBOL),$(ARCH_ENTRY_SYMBOL))
KERNEL_LD_DEFS = -DARCH_NAME='"$(ARCH_NAME)"' -DENTRY_SYMBOL=$(KERNEL_ENTRY_SYMBOL) \
                 -DKERNEL_BASE=$(KERNEL_LOAD_ADDR) -DPAGE_SIZE=$(ARCH_PAGE_SIZE) \
                 $(LDSCRIPT_DEFS)

$(KERNEL_LD): $(LDSCRIPT_TEMPLATE)
	@mkdir -p $(dir $@)
	$(CPP) $(KERNEL_LD_DEFS) $< -o $@

# ---- 头文件依赖 -------------------------------------------------------------
# 编译时带了 -MD, 于是每个 .o 旁边会生成一个 .d 文件, 记录它包含了
# 哪些头文件。但**必须显式 include 这些 .d 文件**, 依赖才会生效。
#
# 【不 include 的后果 (本项目真实踩到过两次)】
#   改了 include/uapi/syscall.h 或 platform.h 之后重新 make, 目标文件
#   不会重建 —— 现象是"我明明改对了, 但行为一点没变", 而 make clean
#   之后又对了。这种"改动被静默忽略"的问题比编译错误麻烦得多。
#
# 这里显式列出所有会生成 .d 的目标文件。新增一类目标文件时记得加进来。
ALL_DEPS := $(KERNEL_OBJS:.o=.d)
-include $(ALL_DEPS)


# ---- 6. 编译规则 -----------------------------------------------------------
# 用 mkdir -p 自动创建目标目录, 于是不需要一成不变地手写一长串 mkdir
# (2025 版本里那个列出 11 个 mkdir 的 $(TARGET) 规则很容易漏)。
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# ---- 7. 链接内核 -----------------------------------------------------------
# 链接脚本由模板生成 (见上面的 KERNEL_LD 规则): 架构名、入口符号、页大小
# 来自 mk/arch/*.mk, 基址来自 configs/*.mk 的 KERNEL_LOAD_ADDR, 启动协议
# 可以用 LDSCRIPT_DEFS 追加自己需要的段。
#
# "链接结果是否符合预期"由生成的脚本里的 ASSERT 负责 (它比在 Makefile 里
# 用 objdump 比较地址更靠近事实, 而且失败时链接器会直接说出原因)。
$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_LD)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -T $(KERNEL_LD) $(KERNEL_OBJS) -o $@
	@echo "  [ld] $@ (基地址 $(KERNEL_LOAD_ADDR), 脚本: $(LDSCRIPT_TEMPLATE))"

# ---- 10. 主构建目标 --------------------------------------------------------
.PHONY: build
build: $(KERNEL_ELF)
	@echo ""
	@echo "===== 构建成功 ====="
	@echo "  配置      : $(CONFIG)"
	@echo "  内核      : $(KERNEL_ELF)"
	@echo ""

.PHONY: kernel
kernel: $(KERNEL_ELF)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "已清理 $(BUILD_DIR)"

# 清理所有配置的产物
.PHONY: clean-all
clean-all:
	rm -rf build
	@echo "已清理全部构建产物"
