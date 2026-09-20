TOOLPREFIX ?= riscv64-elf-
CC := $(TOOLPREFIX)gcc
OBJCOPY := $(TOOLPREFIX)objcopy
HOSTCC ?= cc
BUILD := build/$(CONFIG)
CFLAGS := -std=gnu11 -O2 -g -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -mcmodel=medany -mno-relax -march=rv64gc -mabi=lp64d -MMD -MP
CPPFLAGS := -Iinclude -I. -Iarch/$(ARCH)/include -Iplatform/$(PLATFORM)
