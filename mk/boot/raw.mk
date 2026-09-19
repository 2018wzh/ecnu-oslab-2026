# ============================================================================
# mk/boot/raw.mk -- 裸机直启 (没有固件, 自己从 M-mode 降到 S-mode)
# ----------------------------------------------------------------------------
# 【这是"零修改适配新启动协议"的活证据】
# 本文件与 mk/boot/sbi.mk 的差异是: 入口汇编、早期控制台、从核启动、
# 时钟设置各换一份实现 —— 全部是**新增文件**, 没有任何一个 kernel/ 下的
# 文件因为这条路径而改动。
#
# 【怎么跑】
#   make CONFIG=riscv64-qemu-virt-raw run
# 配置里用 `-bios none`, 于是 QEMU 把内核加载到链接地址并在 **M-mode**
# 直接跳到入口 —— 固件那一层被彻底拿掉了。
#
# 【这条路径教会你什么】
# 有固件时, 下面这些事都是固件做的, 你从来不用想:
#   PMP 配置、异常/中断委派、Sstc 开关、时钟中断怎么进 S-mode、
#   怎么启动其他核。
# 没有固件时, 它们全部变成你的工作 —— 这正是"开发板移植"的核心内容,
# 也是本实验要求学生手工完成的部分 (见 docs/porting.md)。
# ============================================================================

BOOT_NAME := 裸机直启

# 入口: 先跑 M-mode 过渡 (raw/entry_m.S), 它 mret 到共用的 S-mode 入口
BOOT_SSRCS := \
	$(ARCH_BOOT_DIR)/raw/entry_m.S \
	$(ARCH_BOOT_DIR)/entry.S

BOOT_CSRCS := \
	$(ARCH_BOOT_DIR)/raw/early_console.c \
	$(ARCH_BOOT_DIR)/raw/smp_none.c \
	$(ARCH_BOOT_DIR)/raw/time_sstc.c

BOOT_ENTRY_SYMBOL := _entry_m

BOOT_LD_DEFS :=

# 没有固件 = 没有 HSM 扩展: 固件启动的其他 hart 只能停车, 不能当从核用
# (它们会由内核的 HSM 调用启动 —— 而这里没有 HSM)。见 entry.S 里的用法。
BOOT_CFLAGS := -DCONFIG_BOOT_RAW=1 -DBOOT_NAME='"裸机直启"' -DCONFIG_BOOT_LOSERS_PARK=1
