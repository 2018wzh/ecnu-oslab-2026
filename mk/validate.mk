# ============================================================================
# mk/validate.mk -- 配置三项 (ARCH / PLATFORM / BOOT) 的落地检查
# ----------------------------------------------------------------------------
# 只做一件事: 确认这三个名字各自有对应的 mk 文件与平台头文件。
# 没有这一层的话, 拼错一个名字会一路走到编译期才报出难以理解的错误。
# ============================================================================

ifeq ($(wildcard mk/arch/$(ARCH).mk),)
  $(error 未知 ARCH='$(ARCH)' (缺少 mk/arch/$(ARCH).mk))
endif
ifeq ($(wildcard mk/platform/$(PLATFORM).mk),)
  $(error 未知 PLATFORM='$(PLATFORM)' (缺少 mk/platform/$(PLATFORM).mk))
endif
ifeq ($(wildcard mk/boot/$(BOOT).mk),)
  $(error 未知 BOOT='$(BOOT)' (缺少 mk/boot/$(BOOT).mk))
endif
ifeq ($(wildcard $(PLATFORM_DIR)/platform.h),)
  $(error 平台 '$(PLATFORM)' 缺少 $(PLATFORM_DIR)/platform.h)
endif

# 【工具链前缀必须由架构提供】
# 它是"加一个架构要提供什么"的一份清单里最容易被漏掉的一项, 而漏掉的
# 后果不是报错而是**静默地用错工具链** (见 mk/common.mk 的说明)。
# 放在这里检查是因为本文件在最后被 include —— 此时 mk/arch/*.mk 已经生效。
ifeq ($(strip $(ARCH_TOOLPREFIX)),)
  $(error 架构 '$(ARCH)' 没有提供 ARCH_TOOLPREFIX (见 mk/arch/$(ARCH).mk))
endif

# 配置横幅。`make list` / `make help` 本身就是用来看可用选项的, 不需要它。
ifneq ($(filter list help,$(MAKECMDGOALS)),)
else
$(info [config] ARCH=$(ARCH) PLATFORM=$(PLATFORM) BOOT=$(BOOT) LOAD=$(KERNEL_LOAD_ADDR))
endif
