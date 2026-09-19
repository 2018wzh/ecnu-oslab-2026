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
	kernel/trap/trap.c \
	kernel/trap/timer.c \
	kernel/mm/uvmmap.c \
	kernel/proc/proc.c \
	kernel/proc/init.c \
	kernel/proc/initcode_blob.c \
	kernel/proc/exec.c \
	kernel/syscall/syscall.c \
	kernel/syscall/sysfunc.c \
	kernel/fs/file.c \
	kernel/mm/uvm.c \
	kernel/sched/sched.c \
	kernel/sync/sleeplock.c \
	kernel/fs/block.c \
	kernel/fs/bio.c \
	kernel/fs/bitmap.c \
	kernel/fs/inode.c \
	kernel/fs/dentry.c \
	kernel/fs/device.c \
	kernel/fs/fs.c \
	kernel/proc/elf.c \

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
USER_LD   := $(BUILD_DIR)/user.ld

LDSCRIPT_DEFS ?=
# 入口符号: 启动协议可以覆盖 (见 mk/boot/raw.mk)
KERNEL_ENTRY_SYMBOL = $(if $(BOOT_ENTRY_SYMBOL),$(BOOT_ENTRY_SYMBOL),$(ARCH_ENTRY_SYMBOL))
KERNEL_LD_DEFS = -DARCH_NAME='"$(ARCH_NAME)"' -DENTRY_SYMBOL=$(KERNEL_ENTRY_SYMBOL) \
                 -DKERNEL_BASE=$(KERNEL_LOAD_ADDR) -DPAGE_SIZE=$(ARCH_PAGE_SIZE) \
                 $(LDSCRIPT_DEFS)
USER_LD_DEFS   = -DARCH_NAME='"$(ARCH_NAME)"' -DENTRY_SYMBOL=$(USER_ENTRY) \
                 -DUSER_BASE=$(USER_BASE) -DPAGE_SIZE=$(ARCH_PAGE_SIZE)

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
ALL_DEPS += $(USER_INIT_OBJ:.o=.d) $(USER_LIB_OBJ:.o=.d) $(USER_TEST_OBJ:.o=.d)
-include $(ALL_DEPS)

$(USER_LD): $(USER_LD_TEMPLATE)
	@mkdir -p $(dir $@)
	$(CPP) $(USER_LD_DEFS) $< -o $@

.SECONDARY: $(USER_LIB_OBJ) $(USER_TEST_OBJ)

# ---- 5. 用户程序 -----------------------------------------------------------
USER_INIT_C  := user/initcode.c
USER_LIB_C   := user/syscall.c user/help.c
USER_TEST_C  := $(filter-out $(USER_INIT_C) $(USER_LIB_C), $(wildcard user/test_*.c))

USER_INIT_OBJ := $(BUILD_DIR)/user/initcode.o
USER_LIB_OBJ  := $(patsubst %.c,$(BUILD_DIR)/%.o,$(USER_LIB_C))
USER_TEST_OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(USER_TEST_C))
USER_TEST_ELF := $(patsubst %.c,$(BUILD_DIR)/%.elf,$(USER_TEST_C))

INITCODE_H := $(BUILD_DIR)/user/initcode.h

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

# ---- 8. 用户程序规则 ---------------------------------------------------
# 用户程序的编译/链接规则, 以及 initcode 的嵌入。
# 这一段与 user/ 一起在 lab-4 引入 —— 构建脚本的分段标记同时也是
# "这个阶段有哪些东西" 的索引, 阶段工具靠它裁剪出每个 lab 分支。
$(BUILD_DIR)/user/%.o: user/%.c
	@mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/user/%.elf: $(BUILD_DIR)/user/%.o $(USER_LIB_OBJ) $(USER_LD)
	@# 【-N (--omagic): 让段不要按页对齐】
	@# 不加它的话, 链接器会把第一个 LOAD 段的文件偏移推到 0x1000
	@# (与它的虚拟地址相同), 于是每个用户 ELF 都白白多出 4 KB 的
	@# 空洞。而内核的 exec 先把整个文件读进**一个物理页**(见
	@# kernel/syscall/sysfunc.c 的 sys_exec), 4 KB 的空洞会让任何
	@# 用户程序都超过上限 —— 现象是"exec 返回 E_BADFMT", 而文件本身
	@# 完全正常。
	$(LD) $(LDFLAGS) -N -T $(USER_LD) $< $(USER_LIB_OBJ) -o $@
	@# 去掉调试信息与符号表: 用户程序会被打进磁盘镜像,
	@# 带上调试段会让镜像无谓地膨胀几十倍 (且 mkfs 需要逐字节搬运)。
	@# 保留一份未 strip 的副本用于 gdb 调试。
	@cp $@ $@.debug
	$(OBJCOPY) --strip-all $@

# initcode: 内核要把它作为第一个用户进程, 以二进制形式嵌进内核镜像
# initcode 的链接地址必须与内核加载它的地址【完全一致】。
# 原因: initcode 里使用的是 PC 相对寻址 (auipc/addi), 如果链接时
# 假设自己在 0x0、实际被加载到 0x1000, 那么所有取字符串、跳转的
# 地址都会偏移 0x1000, 结果就是跳到未映射的地址 (取指缺页 at 0x0)。
#
# 为什么不用 0x0: 我们故意不映射第 0 页, 这样用户程序里的
# "空指针解引用"会立刻触发缺页异常, 让 bug 尽早暴露, 而不是
# 静默地访问到有效内存。
INITCODE_ADDR := 0x1000

$(INITCODE_H): $(USER_INIT_OBJ) $(USER_LD)
	@mkdir -p $(dir $@)
	@# 【注意不要用 -N (--omagic)】
	@# -N 会强制把 text 段放到地址 0 并关闭页对齐, 它会让
	@# 下面的 -Ttext 失效 —— 链接结果显示 VMA=0, 而内核按 0x1000
	@# 加载, 于是 PC 相对寻址全部偏移, 表现为"取指缺页 at 0x0"。
	@# 这是很隐蔽的一类链接脚本/链接选项冲突: 命令行看着对, 结果不对。
	$(LD) $(LDFLAGS) -e main -Ttext $(INITCODE_ADDR) \
		-o $(BUILD_DIR)/user/initcode.out $<
	$(OBJCOPY) -S -O binary $(BUILD_DIR)/user/initcode.out $(BUILD_DIR)/user/initcode
	@cd $(BUILD_DIR)/user && xxd -i initcode > initcode.h
	@echo "  [embed] initcode.h (链接地址 $(INITCODE_ADDR))"

# initcode 的加载地址由构建系统传给内核, 保证链接地址与加载地址
# 来自同一个来源, 不会出现"两边各写一个数字然后不一致"的问题。
CFLAGS += -DINITCODE_LOAD_ADDR=$(INITCODE_ADDR)

# ---- 9. 磁盘镜像 -----------------------------------------------------------
DISKIMG := $(BUILD_DIR)/disk.img
MKFS    := $(BUILD_DIR)/host/mkfs

$(MKFS): tools/mkfs/mkfs.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOSTCFLAGS) -Itools/mkfs -o $@ $<
	@echo "  [host-cc] mkfs  (注意: 用宿主编译器, 不是交叉编译器)"

$(DISKIMG): $(MKFS) $(USER_TEST_ELF)
	$(MKFS) $@ $(USER_TEST_ELF)
	@echo "  [mkfs] $@"

# ---- 9.1 initcode 嵌入 -----------------------------------------------------
# 把生成的 initcode.h 放到 include 路径下, 这样 kernel/proc/exec.c
# 只要 #include <initcode.h> 就能拿到数组。
#
# 【为什么要放到头文件搜索路径而不是用相对路径】
# 因为它的位置取决于构建配置 (build/<variant>/user/initcode.h),
# 写死相对路径会让源码依赖具体的产物目录结构。
# 用 -I 注入可以让源码保持"我只依赖一个名为 initcode.h 的东西"。
GEN_INCLUDE := $(BUILD_DIR)/gen-include

$(GEN_INCLUDE)/initcode.h: $(INITCODE_H)
	@mkdir -p $(GEN_INCLUDE)
	@cp $(INITCODE_H) $@

# exec.c 依赖生成的 initcode.h, 头文件变了要重新编译
$(BUILD_DIR)/kernel/proc/initcode_blob.o: $(GEN_INCLUDE)/initcode.h

CFLAGS += -I$(GEN_INCLUDE)

# ---- 10. 主构建目标 --------------------------------------------------------
.PHONY: build
build: $(KERNEL_ELF) $(INITCODE_H) $(DISKIMG)
	@echo ""
	@echo "===== 构建成功 ====="
	@echo "  配置      : $(CONFIG)"
	@echo "  内核      : $(KERNEL_ELF)"
	@echo "  磁盘镜像  : $(DISKIMG)"
	@echo ""
	@echo "  下一步: make CONFIG=$(CONFIG) run"

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
