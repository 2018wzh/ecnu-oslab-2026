# 移植手册：给内核增加一块新开发板

> 本手册的目标不是"让内核在另一块板子上跑起来"，而是让你能回答：
> **"为什么只需要改这些地方？"**

---

## 0. 先理解你要做的事情有多小

在本项目的设计里，增加一块新开发板应当**只涉及 `platform/`、`configs/`
和（必要时）`drivers/`**，而 `kernel/` 与 `arch/riscv64/` **一行都不用改**。

这不是巧合，而是分层的直接结果：

```
kernel/      ← 只调用抽象接口，不知道任何具体地址
    ↓
arch/        ← 只管 CPU 机制，不知道内存映射
    ↓
platform/    ← 所有"这台机器长什么样"的知识都在这里   ★ 你要改的地方
    ↓
drivers/     ← 只管设备协议，地址由 platform 提供
```

**验收标准**：移植完成后，运行

```bash
git diff --stat lab-9 -- kernel/ arch/     # lab-9 换成你开始移植时的那个分支
```

如果输出里出现了 `kernel/` 或 `arch/` 下的文件，请说明每一处为什么
无法避免。这些地方就是"抽象泄漏点"，是很好的实验报告素材。

---

## 1. 收集新板子的信息

在写任何代码之前，先准备好下面这张表。缺任何一项都会导致后面卡住。

| 项目 | 从哪里得到 | 本项目中的名字 |
|---|---|---|
| DRAM 起始地址 | SoC 手册的内存映射章节 | `PLAT_DRAM_BASE` |
| DRAM 可用大小 | 板载内存规格 | `PLAT_DRAM_SIZE` |
| 内核加载地址 | 固件跳转地址（通常是 DRAM 基址 + 2MB） | `PLAT_KERNEL_BASE` |
| CPU 数量 | SoC 规格 | `PLAT_NCPU` |
| 内核使用哪些 hart | 哪些核被固件/监控核占用 | `PLAT_HART_MIN` / `MAX` / `BOOT_HART` |
| 串口基地址 | SoC 手册 | `PLAT_UART0_BASE` |
| 串口输入时钟 | SoC 手册（**用于算波特率分频**） | `PLAT_UART0_CLOCK` |
| 中断控制器基地址 | 通常是 PLIC | `PLAT_PLIC_BASE` |
| 时钟频率 / 定时器源 | SoC 手册 | `PLAT_TIMER_INTERVAL` |
| 存储控制器类型与地址 | SD 卡？eMMC？NVMe？ | `PLAT_SDIO0_BASE` |

### 关于 `PLAT_HART_MIN` / `MAX`：最容易搞错的一项

不要想当然地认为"内核从 hart 0 开始"。真实 SoC 常常不是：

- **QEMU virt**：hart 0..1 全部可用，`BOOT_HART = 0`，区间 `[0, 1]`
- **VisionFive2 (JH7110)**：hart 0 是 S7 监控核，被 U-Boot 占用；
  内核只能用 hart 1..4，`BOOT_HART = 1`，区间 `[1, 4]`

如果这里写错，症状是**"4 核的板子只起来 3 个核"**，而且不会有任何报错 ——
在真机上极难发现。

本项目在 `arch/riscv64/smp/cpu.c` 里加了编译期断言来兜住这类错误：

```c
STATIC_ASSERT(PLAT_HART_MAX - PLAT_HART_MIN + 1 == PLAT_NCPU,
              "platform.h 中 PLAT_HART_MAX/MIN 与 PLAT_NCPU 不一致");
STATIC_ASSERT(PLAT_BOOT_HART >= PLAT_HART_MIN && PLAT_BOOT_HART <= PLAT_HART_MAX,
              "platform.h 中 PLAT_BOOT_HART 不在内核可用的 hart 区间内");
```

**移植新板子时，这两个断言是你最先应该让它通过的东西。**

---

## 2. 创建平台头文件

```bash
mkdir -p platform/<board>
cp platform/visionfive2/platform.h platform/<board>/platform.h
```

然后逐项修改。**请把每个值都改成从手册查到的真实值，并写清出处**
（写在注释里，例如 `/* JH7110 TRM, Ch.2 Memory Map, Table 2-1 */`）。
将来别人排查问题时，这条注释能省下几个小时。

### 必须定义的项目清单

```c
/* 内存 */
#define PLAT_DRAM_BASE
#define PLAT_DRAM_SIZE
#define PLAT_KERNEL_BASE
#define PLAT_FIRMWARE_BASE

/* CPU */
#define PLAT_NCPU
#define PLAT_BOOT_HART
#define PLAT_HART_MIN
#define PLAT_HART_MAX

/* 中断控制器 */
#define PLAT_PLIC_BASE
#define PLAT_PLIC_SIZE

/* 时钟 */
#define PLAT_CLINT_BASE          /* 即使不用 MMIO, 读 mtime 也可能需要 */
#define PLAT_TIMER_INTERVAL      /* 必须对应"约 0.1 秒", 见下 */
#define PLAT_TIMER_FREQ          /* 仅在需要自己算间隔时使用 */

/* 串口 */
#define PLAT_UART0_BASE
#define PLAT_UART0_IRQ
#define PLAT_UART0_CLOCK         /* ★ 漏了它串口会输出乱码 */

/* 存储 (如果该平台有) */
#define PLAT_SDIO0_BASE / PLAT_VIRTIO0_BASE

/* 标识 */
#define PLAT_NAME
```

### `PLAT_TIMER_INTERVAL` 的语义很重要

上层代码假设"每约 0.1 秒触发一次时钟中断"。所以这个值必须
**按该平台的时钟频率换算**：

```
PLAT_TIMER_INTERVAL = 时钟频率 / 10
```

| 平台 | 时钟频率 | 间隔值 |
|---|---|---|
| QEMU virt | 10 MHz | 1,000,000 |
| VisionFive2 | 4 MHz | 400,000 |

**数值不同，但语义一致** —— 这正是平台层应当吸收的差异。
如果你直接把 QEMU 的 1000000 抄到一块 4MHz 时钟的板子上，
时钟会走得慢 2.5 倍（表现为 `timer_wait` 等待时间不对）。

### 有条件定义的项

块设备相关的宏用 `#ifdef` 保护即可，`drivers/irqchip/plic.c` 会自动
识别哪些中断源存在：

```c
#ifdef PLAT_UART0_IRQ
#define HAS_UART_IRQ 1
#else
#define HAS_UART_IRQ 0
#endif
```

---

## 3. 创建平台初始化文件

```bash
cp platform/visionfive2/platform.c platform/<board>/platform.c
```

`platform_init()` 的职责**应当尽量少**，因为大部分平台信息是编译期
静态宏，不需要运行时"发现"。通常只需要：

1. 打印板级信息（便于确认内核真的认识这块板子）
2. 如果该板子有特殊的早期初始化（时钟树、PLL、引脚复用），放在这里
3. 提供次核的启动入口地址

**不要**在这里做内存分配器、页表等通用初始化 —— 那些属于 `kernel/`。

---

## 4. 创建构建描述

```bash
cp mk/platform/visionfive2.mk mk/platform/<board>.mk
```

```makefile
PLATFORM_DIR := platform/<board>

PLATFORM_CSRCS := \
	$(PLATFORM_DIR)/platform.c

PLATFORM_INCLUDES := -I$(PLATFORM_DIR)

# ★ 这里决定"复用哪些驱动、新增哪些驱动"
PLATFORM_DRIVERS := \
	drivers/irqchip/plic.c \
	drivers/timer/sbi_timer.c \
	drivers/serial/uart16550.c \
	drivers/block/sdhci.c
```

### 驱动复用判断表

先看新板子的设备**协议**是否与已有的相同，而不是看型号：

| 设备 | 判断依据 | 能否复用 |
|---|---|---|
| 串口 | 寄存器布局是否 16550 兼容（RHR/THR/IER/LCR/LSR 偏移） | 大多数 SoC 是 → 复用 `uart16550.c` |
| 中断控制器 | 是否 SiFive PLIC（优先级/使能/claim 的偏移） | 很多 RISC-V SoC 是 → 复用 `plic.c` |
| 定时器 | 能否读 `mtime` / 能否用 SBI 设置定时 | 优先用 `sbi_timer.c`（最可移植） |
| 存储 | VirtIO / SDHCI / NVMe / 裸 SPI Flash | 协议不同就**必须**写新驱动 |

**关键点**：驱动代码里不允许出现具体地址。检查你的新驱动：

```bash
grep -n '0x[0-9a-fA-F]\{6,\}' drivers/<你的新驱动>.c
```

应当只有寄存器**偏移**（如 `0x044`），不应有**基地址**。

---

## 5. 创建配置档案

```bash
cp configs/riscv64-visionfive2-uboot.mk configs/riscv64-<board>-<boot>.mk
```

```makefile
CONFIG_NAME := riscv64-<board>-<boot>

ARCH     := riscv64
PLATFORM := <board>
BOOT     := uboot          # 或 sbi

PLAT_NCPU ?= 4
KERNEL_LOAD_ADDR := 0x40200000    # 必须与 platform.h 的 PLAT_KERNEL_BASE 一致

UBOOT_IMAGE_FORMAT := fit
UBOOT_LOAD_ADDR    := $(KERNEL_LOAD_ADDR)
UBOOT_ENTRY_ADDR   := $(KERNEL_LOAD_ADDR)

BUILD_VARIANT := $(CONFIG_NAME)
```

### ⚠️ 地址一致性：最容易出错的地方

`KERNEL_LOAD_ADDR`（构建系统）和 `PLAT_KERNEL_BASE`（平台头文件）
**必须是同一个值**。它们不一致时，内核会被链接到一个地址、
却被加载到另一个地址，所有绝对地址访问全部偏移，**一进去就跑飞**，
而且完全没有输出，非常难查。

本项目有两道防线：
1. 链接后断言（`mk/build.mk`）：检查 `.text` 起始地址是否等于期望值
2. 运行期检查（`arch/riscv64/boot/start.c`）：比较 `_entry` 与
   `PLAT_KERNEL_BASE`，不一致时明确报错并停下

移植时如果卡在这里，先确认这两处检查是否通过。

---

## 6. 如果你用的是不同的启动方式

启动路径（`BOOT`）与平台是两个独立维度。目前支持：

- `sbi` — OpenSBI 直接跳转（QEMU `-bios default`）
- `uboot` — U-Boot 解析 FIT 镜像后 `bootm`

**两者的内核入口约定完全相同**（RISC-V S-mode 启动 ABI）：
`a0 = hartid`，`a1 = DTB 地址`，`satp = 0`，`sp` 未定义。
所以 `arch/riscv64/boot/entry.S` 和 `start.c` 不需要改。

真正不同的只是**镜像格式**：

| | OpenSBI 直接加载 | U-Boot 加载 |
|---|---|---|
| 镜像 | ELF（`-kernel` 直接吃） | FIT（`.itb`） |
| 谁负责搬运 | QEMU/固件 | U-Boot 按 FIT 声明的地址搬运 |
| 校验 | 无 | FIT 头里有架构与校验和 |

如果新板子用别的 bootloader（UEFI、coreboot、自定义），
你需要一个 `mk/boot/<名字>.mk` 说明它的镜像格式，以及
（如果入口约定不同）一组新的 `BOOT_CSRCS`/`BOOT_SSRCS`。

**注意**：`kernel/` 不应该因为换了 bootloader 而改变。

---

## 7. 验证清单

按顺序做，每步都确认后再进行下一步：

```bash
# 1. 配置是否被识别
make list | grep <board>
make CONFIG=riscv64-<board>-<boot> info

# 2. 确认选中了正确的平台与驱动
make CONFIG=riscv64-<board>-<boot> info | grep -E 'platform/|drivers/'
#    ^ 应当看到 platform/<board>/platform.c，以及你期望复用的驱动

# 3. 编译（这一步能抓出宏缺失、断言失败）
make CONFIG=riscv64-<board>-<boot> build

# 4. 确认链接地址正确（构建系统会自动断言，这里手动再确认一次）
riscv64-elf-nm build/riscv64-<board>-<boot>/kernel.elf | grep ' _entry$'
#    ^ 应当等于 configs 里的 KERNEL_LOAD_ADDR

# 5. 如果有 QEMU 或模拟器支持，先在那里跑通
make CONFIG=riscv64-<board>-<boot> run      # 仅当 PLATFORM 是 qemu-virt 类

# 6. 生成可交付镜像
make CONFIG=riscv64-<board>-<boot> image

# 7. 上真机验证（见 docs/board-deploy.md）
```

### 真机启动排查顺序

按这个顺序查，**不要跳步**——每一步都在缩小问题范围：

| 现象 | 最可能的原因 | 查什么 |
|---|---|---|
| 完全没有任何输出 | 串口地址错 / 时钟频率错 / 内核加载地址错 | 先用固件的输出确认串口能通（U-Boot 有输出说明串口硬件没问题） |
| 输出乱码 | `PLAT_UART0_CLOCK` 不对，波特率分频算错 | 换几个常见频率（24M/48M/3.6864M）试试 |
| 输出几行后卡死 | 页表映射不全（尤其是设备区没映射） | 检查 `kvm_init()` 是否映射了所有 MMIO 区域 |
| 少启动一个核 | `PLAT_HART_MIN/MAX/NCPU` 不自洽 | 编译期断言应当已经拦住 |
| 时钟走得快/慢 | `PLAT_TIMER_INTERVAL` 与时钟频率不匹配 | 按"频率/10"重算 |
| 打开分页后立刻无输出 | `satp` 写入了物理地址而不是 PPN | 检查 `arch_mmu_activate()` 是否做了 `>> 12` |

---

## 8. 这次移植训练了什么

做完之后，请对照回答：

1. **你改了哪些文件？** 其中有多少在 `kernel/` 下？
2. **`plic.c` 和 `uart16550.c` 为什么能直接复用？**
   （提示：它们依赖的是宏，而不是具体数值）
3. **如果你把 UART 地址直接写进 `drivers/serial/uart16550.c`，
   会发生什么？** 试着改一下，看编译和运行会怎样。
4. **`PLAT_TIMER_INTERVAL` 在两个平台上数值不同但语义相同 ——
   这个"数值不同、语义一致"的边界应该划在哪一层？为什么？**
5. **有没有哪一处你不得不修改 `kernel/`？**
   那说明现有抽象有缺陷。你会怎么改进这个接口，
   使得下一次移植不需要再改它？

第 5 题没有标准答案，但它正是真实内核开发者每天在做的事。

---

## 附：关于设备树（DTB）

本项目**刻意没有使用设备树**。这是一个教学取舍：

- **设备树解决的真实问题**：一块 SoC 有几十种板级变体（内存大小、
  外设接法、引脚复用各不相同），不可能为每种变体编译一个内核。
  所以把硬件描述从内核里拿出来，变成启动时传入的数据。
- **为什么不在这里用**：它引入了一个完整的解析器（约 1000 行）和
  一套描述语言（DTS），会把注意力从"分层边界"转移到"解析器实现"。
- **两者解决的是同一个问题**：硬件信息属于平台层。
  静态头文件是"编译期确定平台"，设备树是"运行期确定平台"。

如果你对设备树感兴趣，一个合适的扩展练习是：
在 `platform/<board>/platform.c` 里解析 `a1` 指向的 DTB，
用其中的 `/memory` 节点覆盖 `PLAT_DRAM_SIZE`。
你会立刻发现这需要一个新的抽象层——那正是 Linux 里
`drivers/of/` 存在的原因。
