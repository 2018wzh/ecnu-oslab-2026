# LAB-1: 机器启动

**前言**

本次实验从机器启动开始。你需要让多个 CPU 进入内核，完成初始化，通过串口输出信息。QEMU 使用 OpenSBI，VisionFive2 使用 U-Boot/OpenSBI；固件已经完成进入 S-mode 之前的工作。

## 1. 代码组织结构

```text
Makefile、configs/、mk/          构建、运行与调试 (NEW)
arch/riscv64/
  boot/entry.S                 内核入口与每核初始栈 (NEW)
  boot/start.c                 进入 C 后的启动工作 (TODO)
  boot/sbi.c                   OpenSBI 调用封装 (NEW)
  cpu.c、include/asm/          CPU 身份与中断操作 (NEW)
  early_trap.S                 早期异常停车入口 (NEW)
  linker/kernel.lds.S          内核链接布局 (NEW)
platform/                      硬件参数 (NEW)
drivers/serial/                UART 驱动 (NEW)
include/kernel/                接口声明 (NEW)
kernel/
  main.c                       双核主流程 (TODO)
  lib/console.c                串口连接与紧急输出 (NEW)
  lib/print.c                  格式化输出与断言 (TODO)
  lock/spinlock.c              自旋锁 (TODO)
tools/fitgen.c                 FIT 镜像打包 (NEW)
```

`NEW` 是教师完整提供的新代码，`TODO` 是学生任务；后续的 `CHANGE` 表示为支持当前实验修改的旧文件。

## 2. 实验核心目标

完成双核启动，进入 `main()` 并输出：

```text
cpu 0 is booting!
cpu 1 is booting!
```

先后顺序可以不同，但每核只打印一次，字符不能交错。QEMU 默认两核；VisionFive2 使用 hart 1～4。

## 3. 具体任务

| 文件 | 本章任务 |
|---|---|
| `arch/riscv64/boot/start.c` | `start` |
| `kernel/main.c` | `main`：初始化、启动其他核、同步 |
| `kernel/lib/print.c` | `printf`、`assert` |
| `kernel/lock/spinlock.c` | `spinlock_init/holding/acquire/release` |

### 3.1 机器启动本身

要想实现上述核心目标，仔细想想只需要完成两件事：

1. 让内核在 QEMU 上跑起来：`entry.S` 到 `start.c` 到 `main.c`。
2. 让内核向屏幕输出一些字符串，也就是实现 C 语言中经常调用的 `printf()`。

阅读启动汇编和链接脚本，理解每核初始栈以及汇编如何调用 C。`a0` 是 hartid；冷启动的 `a1` 是 DTB 地址，HSM 从核的 `a1` 是启动参数，不能混为一谈。

汇编提供栈建立、CPU 身份保存和冷启动 BSS 清零。`start()` 调用 `arch_early_init()`，保持分页与中断关闭、安装早期异常入口，再进入 `main()`。不需要实现 M-mode 到 S-mode 切换。

主核调用 `print_init()`，通过 `arch_start_cpu()` 启动其他 CPU，并检查返回值。其他核等待公共初始化完成再打印。请使用原子操作及合适的内存顺序，不能只用普通变量或 `volatile`。

hartid 是硬件编号，cpuid 是内核从 0 开始的连续编号。用 `arch_is_boot_cpu()` 判断主核，不要假定 hart 0。完成输出后调用 `arch_park()`。

### 3.2 printf 面临的资源竞争问题

串口是一种设备资源，`printf()` 输出字符本质是在一段时间内持有它。输出 `"hello,world!"` 需要连续调用多次字符输出。如果另一个执行流打印 `"hello,os!"`，可能出现：

```text
# 混乱
hellohello,,world!os!
hheelllloo,,wosrld!!
# 有序
hello,world!hello,os!
hello,os!hello,world!
```

生活中的例子：公共卫生间通过“门锁”保证资源在一段时间内只被一人独占。映射到操作系统，最简单的资源锁就是自旋锁。

```c
spinlock_t lk;
spinlock_init(&lk, "print");
spinlock_acquire(&lk);
console_putc('O');
console_putc('S');
spinlock_release(&lk);
```

关闭本核中断避免中断处理重复请求同一把锁；原子操作避免多个 CPU 同时获取成功。教师提供带嵌套计数的 `push_off/pop_off`，请先理解如何恢复原中断状态，再完成锁。`locked` 使用原子内建操作，不混用无同步普通访问。

### 3.3 格式化输出与断言

先读 UART 和 `console_putc()`。数字转换 helper 已提供，学生完成格式分派和整次输出加锁：

| 格式 | 参数 |
|---|---|
| `%d` | `int`，有符号十进制 |
| `%x` | `unsigned int`，十六进制 |
| `%p` | `void *`，带 `0x` 前缀 |
| `%c` | 默认提升后的 `int` 字符 |
| `%s` | `const char *` 字符串 |
| `%%` | 百分号，不读参数 |

不要求宽度、精度或长度修饰符。NULL 字符串输出 `(null)`；未知格式原样输出，末尾孤立 `%` 不越界读取。

`assert(condition, warning)` 条件成立时返回，否则调用 `panic()`。紧急输出不依赖学生打印与锁。

## 4. 测试

需要 `riscv64-elf-gcc`、QEMU、OpenSBI；可用 `TOOLPREFIX` 指定兼容交叉工具链。

```bash
make CONFIG=riscv64-qemu-virt-sbi build
make CONFIG=riscv64-qemu-virt-sbi run
make CONFIG=riscv64-qemu-virt-sbi debug
```

调试时执行 `gdb-multiarch build/riscv64-qemu-virt-sbi/kernel.elf`，连接 `target remote :1234`。框架可编译，但未完成函数会报告 TODO 并停止。

检查每核独立栈、初始化一次、从核等待、每核输出一次。测试零、最小负整数、十六进制、指针、字符、字符串、百分号；检查嵌套不同锁后的中断恢复和断言失败路径。

VisionFive2 镜像需要 `dtc`：

```bash
make CONFIG=riscv64-visionfive2-uboot image
```

将 `build/riscv64-visionfive2-uboot/kernel.itb` 放到 FAT 分区，U-Boot 中例如：

```text
fatload mmc 1:1 ${kernel_addr_r} kernel.itb
bootm ${kernel_addr_r}
```

设备/分区号按实际环境调整，串口 115200 8N1。固件需支持 SBI HSM，冷启动只交接一个 hart，其他核由内核启动。镜像生成不等于真机验证。

## 5. 课后实验

### 5.1 并行加法

两个 CPU 对同一计数器各加一百万次。先用分开的原子读和原子写观察丢失更新，再用锁保护整个读、加一、写回。等两核都完成后读取结果，应为 `2000000`。不要用存在数据竞争的普通共享变量作为正确性依据。

简单说明上锁位置的影响，考虑锁的粒度粗细。

### 5.2 并行输出

去掉打印锁，设计测试使输出交错，再恢复比较。一次未发生交错不能证明不存在竞争。测试代码和结果可以附在文档中，不保留在正常启动流程。

### 5.3 进阶目标

- UEFI：了解并尝试另一种内核启动路径。
- DTB 动态发现：读取内存、CPU 和串口信息。
- BootInfo：统一不同启动方式交接的信息。

## 6. 关于代码仓库的维护

教师仓库配置为 `upstream` 后：

```bash
git fetch upstream
git switch -c lab-1 upstream/lab-1
# 完成并提交，例如 lab-1: implement boot and spinlock
```

进入下一实验前提交全部工作：

```bash
git fetch upstream
git switch -c lab-2 lab-1
git rebase --onto upstream/lab-2 upstream/lab-1
```

教师只增加框架，不用答案替换原实现。遇到冲突先理解双方改动。仓库应包含代码和 Markdown 文档，记录功能、思考、实验联系、耗时与队友贡献。总之，这是你的仓库，请对自己的代码和文档负责。代码连续发展，文档记录每次新工作；请及时提交并同步。
