# ============================================================================
# arch/riscv64.mk -- RISC-V 64 位架构的构建描述
# ----------------------------------------------------------------------------
# 由 include 本文件的上层 (Makefile) 负责保证 ARCH == riscv64。
# 本文件只声明"该架构编译哪些源文件、需要哪些编译选项"。
#
# 【加一个架构要提供什么 —— 这份清单就是"架构维度的接口"】
#   新建 mk/arch/<arch>.mk, 提供:
#     ARCH_TOOLPREFIX         交叉工具链前缀 (必需; 缺了 mk/validate.mk 会报错)
#     ARCH_NAME               OUTPUT_ARCH 的机器名 (binutils 的词汇)
#     ARCH_PAGE_SIZE          页大小 (链接脚本的对齐粒度)
#     ARCH_LDSCRIPT_TEMPLATE  内核链接脚本模板的路径
#     ARCH_BOOT_DIR           启动层实现所在的目录
#     ARCH_ENTRY_SYMBOL       默认入口符号 (启动协议可以覆盖)
#     ARCH_CFLAGS             -march/-mabi 这类编译选项
#     ARCH_CSRCS / ARCH_SSRCS 本架构的源文件 (显式列出, 不要用 wildcard)
#     QEMU_ARCH               给 qemu-system-<arch> 用的名字
#     ELF_MACHINE / ELF_DATA  (在 arch/<arch>/include/asm/elf.h 里)
#   再新建 arch/<arch>/{include,linker,boot,mm,trap,smp,process}/, 然后写一份
#   configs/<arch>-<machine>-<boot>.mk。**不需要改 mk/ 下的任何通用文件。**
#
# 【不要预置第二个架构的骨架】
# 空的 arch/aarch64/ 目录比没有目录更糟: 它让人以为"已经支持了", 而
# wildcard 式的构建还会把它的文件链进来。缺什么就报什么, 比假装有更好。
# ============================================================================

# ---- 目标三元组 ------------------------------------------------------------
# 使用 riscv64-elf- (bare-metal) 而不是 riscv64-linux-gnu-。
# 原因: 我们要生成的是没有操作系统依赖的裸机镜像,
#       linux-gnu 工具链默认链接 Linux 的动态加载器和 libc, 不适合裸机。
ARCH_TOOLPREFIX ?= riscv64-elf-

# 本架构的启动层目录。
# 【为什么要有这个变量】
# mk/boot/*.mk 描述的是**启动协议**(固件交接 / 裸机直启), 它与架构正交;
# 但"协议的实现代码放在哪个目录"是架构的事实。以前那三个文件里直接写着
# arch/riscv64/boot/..., 于是"加一个架构"除了新目录还要改启动层的三个
# 文件 —— 那正是"通用层里出现架构字符串"。现在启动层只写
# $(ARCH_BOOT_DIR)/..., 换架构时 mk/boot/*.mk 一行都不用动。
ARCH_BOOT_DIR ?= arch/$(ARCH)/boot

# ---- 架构相关编译选项 ------------------------------------------------------
ARCH_CFLAGS := -march=rv64gc -mabi=lp64d -mcmodel=medany -mno-relax

# ---- 架构源文件 (显式列出, 不使用 wildcard) --------------------------------
# 分析报告特别强调: 不要用 $(wildcard arch/*/*.c) 收集源码。
# 一旦同时存在 riscv64/ 和 aarch64/, wildcard 会把两个架构的目标文件
# 全部链接进来, 产生重复符号。必须由 ARCH 显式决定 object graph。
ARCH_CSRCS := \
	arch/riscv64/boot/start.c \
	arch/riscv64/smp/cpu.c \
	arch/riscv64/smp/irq.c \
	arch/riscv64/trap/time.c \

ARCH_SSRCS := \
	arch/riscv64/trap/early.S \

# 入口汇编 (entry.S) 也归启动维度: 不同协议的进入状态不同
# (S-mode 交接 / M-mode 直启), 见 mk/boot/<boot>.mk 的 BOOT_SSRCS。

# QEMU 的机器名 (qemu-system-<这个>) —— 它跟着架构走
QEMU_ARCH := riscv64

# ---- 架构头文件搜索路径 ----------------------------------------------------
# 这样 generic kernel 可以直接 #include <asm/csr.h> 而不用写长相对路径。
ARCH_INCLUDES := -Iarch/riscv64/include

# ---- 架构事实 (供链接脚本模板与构建规则使用) --------------------------------
# 这三个值以前散落在链接脚本里 (OUTPUT_ARCH("riscv")、ALIGN(0x1000)),
# 现在集中在这里 —— 链接脚本是模板, 由这些参数生成。
ARCH_NAME       := riscv
ARCH_PAGE_SIZE  := 4096

# ---- 链接脚本 --------------------------------------------------------------
# 本架构的标准内核链接脚本 (S-mode 直接启动: 固件把内核放到固定物理地址)。
ARCH_LDSCRIPT_TEMPLATE := arch/riscv64/linker/kernel.ld.in

# 【链接脚本可以被覆盖 —— 这就是"不把 linker 写死"的落点】
#
# `ARCH_LDSCRIPT_TEMPLATE` 是**默认值**, 启动协议可以覆盖 `LDSCRIPT_TEMPLATE`。
# 覆盖它的地方有两处, 都在本文件之后被 include:
#
#   configs/<cfg>.mk   —— 某个具体目标需要特殊布局时
#   mk/boot/<boot>.mk  —— 某个启动协议需要特殊布局时 (推荐写在这里)
#
# 什么时候真的需要覆盖:
#
#   multiboot  x86 的 GRUB 要求文件头 8 KiB 内有一个 multiboot header
#              段, 且要能被 4 字节对齐地找到 —— 段布局与"固件直接跳
#              到入口"完全不同。
#   UEFI       产物是 PE/COFF, 节的对齐与名称都有额外要求。
#   limine     需要 limine requests / base revision 段, 且对段顺序敏感。
#
# 本仓库目前只有 S-mode 固件启动 (OpenSBI / U-Boot 都走这条路),
# 所以还没有第二份脚本。加一个 bootloader 的完整步骤是:
#   1. (可选) 写一份协议自己的模板, 或复用本架构的模板
#   2. 在 mk/boot/<boot>.mk 里 `LDSCRIPT_TEMPLATE := <那份模板>`
#      —— 大多数协议不需要: 它们只要 `BOOT_LD_DEFS` 追加自己的段
#   3. 在 mk/boot/<boot>.mk 里提供该协议需要的入口汇编 (BOOT_SSRCS)
#      与早期控制台 / SMP / 定时器的实现 (BOOT_CSRCS)
# 不需要改 mk/build.mk —— 它只认 `LDSCRIPT_TEMPLATE` 与 `LDSCRIPT_DEFS`。
LDSCRIPT_TEMPLATE ?= $(ARCH_LDSCRIPT_TEMPLATE)

# 入口符号 (ELF 的 e_entry): 默认是本架构的启动汇编入口 `_entry`。
# 启动协议可以覆盖它 —— 例如裸机直启的真实入口是 M-mode 的 _entry_m,
# 因为加载器会跳到 e_entry (见 mk/boot/raw.mk 的 BOOT_ENTRY_SYMBOL)。
ARCH_ENTRY_SYMBOL := _entry
