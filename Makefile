.DEFAULT_GOAL := build
CONFIG ?= riscv64-qemu-virt-sbi
include configs/$(CONFIG).mk
include mk/common.mk
include mk/arch/$(ARCH).mk
include mk/platform/$(PLATFORM).mk
include mk/boot/$(BOOT).mk
include mk/build.mk
include mk/run.mk
include mk/image.mk

.PHONY: help
help:
	@echo 'make [CONFIG=riscv64-qemu-virt-sbi|riscv64-visionfive2-uboot] build|run|debug|image'
