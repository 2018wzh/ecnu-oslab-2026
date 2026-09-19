# ============================================================================
# mk/common.mk -- 工具链与通用编译选项
# ----------------------------------------------------------------------------
# 这里严格区分两类编译器:
#
#   目标编译器 (CROSS_COMPILE): 编译"将要在 RISC-V 上运行"的东西
#                               -> 内核、用户程序
#   宿主编译器 (HOSTCC):        编译"在开发机上运行"的东西
#                               -> tools/mkfs, tools/image
#
# 为什么要区分 (这是一个非常经典的工程错误):
#   mkfs 要把普通 Linux 可执行文件打包成磁盘镜像, 它在你的 x86 开发机上跑。
#   如果你用 riscv64-elf-gcc 去编译 mkfs, 得到的是一份 RISC-V 程序,
#   开发机根本执行不了 —— 报错通常是 "cannot execute binary file"。
# ============================================================================

# ---- 目标工具链 ------------------------------------------------------------
# 【工具链前缀是**架构**的事实, 由 mk/arch/$(ARCH).mk 提供】
#
# 这里用**递归展开** (`=` 而不是 `:=`): arch/*.mk 是在本文件**之后**才被
# include 的, 写 `:=` 的话 `$(ARCH_TOOLPREFIX)` 在这一行就会被展开成空串,
# 于是 CC 变成裸的 "gcc" —— 用宿主 gcc 去编译 RISC-V 内核, 报一堆
# "unrecognized -march" 之类的错, 而真正的原因是**求值时机**。
#
# 【为什么不留一个 riscv64-elf- 的默认值】
# 那正是"通用层里写死了架构字符串": 加一个新架构时, 忘记在 mk/arch/<新架构>.mk
# 里设 ARCH_TOOLPREFIX 不会报错, 而是**静默地用 RISC-V 的工具链去编译**,
# 症状是链接期一堆莫名的架构不匹配。所以这里不给默认值, 由下面
# mk/validate.mk 明确要求提供。
TOOLPREFIX = $(ARCH_TOOLPREFIX)
CC      = $(TOOLPREFIX)gcc
LD      = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
NM      = $(TOOLPREFIX)nm

# ---- 宿主工具链 ------------------------------------------------------------
HOSTCC  ?= gcc
HOSTLD  ?= ld

# ---- 预处理器 (链接脚本模板要用) ----
# 链接脚本也是"源码": 它需要架构名、页大小、基地址这些参数。
# 用 C 预处理器渲染 `.ld.in` 是最省事也最可审查的办法:
# `-P` 去掉行标记, `-x c` 让它按 C 预处理规则处理。
CPP = $(CC) -E -P -x c

# ---- 内核编译选项 ----------------------------------------------------------
# 重要: 这里用 = (递归展开) 而不是 := (立即展开)。
#
# 原因: ARCH_CFLAGS / PLATFORM_INCLUDES / BOOT_CFLAGS 由后面才 include 的
# mk/arch/*.mk、mk/platform/*.mk、mk/boot/*.mk 定义。
# 如果用 := , 这些变量在此时还是空的, 头文件搜索路径就会缺失,
# 表现为莫名其妙的 "platform.h: No such file or directory"。
# 用 = 可以让变量在真正被使用(编译命令展开)时才求值, 此时所有
# 维度的构建描述都已经加载完毕。
CFLAGS = -std=gnu17 -Wall -O2 -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -MD
CFLAGS += -ffreestanding
CFLAGS += -fno-common
CFLAGS += -nostdlib
CFLAGS += -fno-stack-protector
CFLAGS += -fno-pie -no-pie
CFLAGS += $(ARCH_CFLAGS)
# 链接地址来自配置 (configs/*.mk): C 代码用它做一次
# "我被放在哪" 的自检 —— 见 arch/riscv64/boot/start.c 第 7 步。
CFLAGS += -DEXPECTED_LOAD_ADDR=$(KERNEL_LOAD_ADDR)

# 【为什么显式写 -std=gnu17】
#
# 不写的话 GCC 用自己当前的默认方言 —— 那个默认值会随 GCC 版本变化,
# 于是"这份代码用哪个 C 标准"这件事没有答案。显式写出来, 它就是一个
# 明确的事实而不是编译器的偶然选择。
#
# 为什么是 gnu17 而不是严格 ISO 的 c17:
#   内核必须用内联汇编 (`asm volatile("csrw ...")`), 而 `asm` 是
#   GNU 扩展。严格 c17 会定义 __STRICT_ANSI__, 把 `asm` 关键字关掉,
#   于是那 29 处都得改写成 `__asm__`。
#
#   那不是"更标准", 只是换个拼写: `__asm__` 同样是 GNU 扩展, 只是
#   在严格模式下仍然可见。把 29 处 asm 逐字改成 __asm__, 增加的是
#   噪声而不是可移植性 —— 换一个不支持 GNU 扩展的编译器, 两者一样
#   编不过。
#
#   Linux 内核也是这么选的 (-std=gnu11/gnu17)。这是个有意的取舍,
#   写在这里以便学生知道它被权衡过, 而不是随手写的。
#
# 这个选项真正保证的是: **语言的版本是 C17**, 不会漂移到 gnu23 或
# 将来某个 GCC 的新默认值。

# 头文件搜索路径:
#   -Iinclude           -> <kernel/*.h> 与 <uapi/*.h>
#   -Iarch/.../include  -> <asm/*.h>
#   -I$(PLATFORM_DIR)   -> <platform.h>  ★ 平台选择就发生在这里
CFLAGS += -Iinclude
CFLAGS += $(ARCH_INCLUDES)
CFLAGS += $(PLATFORM_INCLUDES)

CFLAGS += $(BOOT_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)

# ---- 链接选项 --------------------------------------------------------------
LDFLAGS = -z max-page-size=4096

# 【为什么显式关掉这个警告】
# 内核的链接脚本把所有段放在**一个** LOAD 段里 (见 arch/<arch>/linker/kernel.ld.in 的说明:
# 教学内核不需要 W^X, 单一映射让"内核在哪个地址"这件事只有一个答案)。
# 于是 ld 会提示 "LOAD segment with RWX permissions"。
#
# 那是**已知且有意**的布局, 不是隐患 —— 但每个 .o 链接时都刷一遍,
# 会把真正需要注意的警告淹掉。学生看不完的警告等于没有警告。
#
# 如果将来把内核拆成 W^X 的多个段, 这一行就该删掉, 让警告重新出现。
LDFLAGS += --no-warn-rwx-segments


# ---- 用户程序编译选项 ------------------------------------------------------
# 用户程序跑在 U-mode, 有自己的 syscall ABI 头文件路径
USER_CFLAGS = -Wall -O2 -ggdb -gdwarf-2 -ffreestanding -nostdlib
USER_CFLAGS += -fno-common -fno-stack-protector -fno-pie -no-pie
USER_CFLAGS += $(ARCH_CFLAGS)
USER_CFLAGS += -Iuser -Iinclude -I$(USER_ARCH_DIR)

# ---- 宿主工具选项 ----------------------------------------------------------
HOSTCFLAGS := -Wall -Werror -O2 -Iinclude
