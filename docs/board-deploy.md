# VisionFive2 开发板部署指南

> 本指南对应 **VisionFive2（昉·星光2，SoC: StarFive JH7110）**。
> 目标平台配置：`CONFIG=riscv64-visionfive2-uboot`

---

## 0. 与 2025 版本的区别

2025 版本的部署方式是：

```makefile
SDcard = /dev/sdb
KERNEL_OFFSET = 20480        # 前 20GB 给 disk.img
sudo dd if=kernel_vf2.bin of=$(SDcard) bs=1M seek=$(KERNEL_OFFSET)
```

然后在 U-Boot 里：

```
mmc read 40200000 2800000 1000
go 40200000
```

**这种方式有几个真实的问题**：

1. **偏移量依赖 SD 卡容量**。`seek=20480`（20GB）要求卡至少 20GB，
   而且如果分区表不同，偏移对应的位置就变了。换一张卡就失效。
2. **裸镜像没有头部信息**。U-Boot 无法校验架构和加载地址，
   地址写错就是静默跑飞（串口一片空白，没有任何提示）。
3. **`go` 是无条件跳转**，不做任何检查。
4. **写错设备名会清掉整块硬盘**（`SDcard = /dev/sdb` 写死在 Makefile 里）。

2026 版本改用 **FIT 镜像 + `bootm`**：

| | 2025（裸 dd + go） | 2026（FIT + bootm） |
|---|---|---|
| 镜像格式 | 裸二进制 | FIT（带 FDT 描述头） |
| 加载地址 | 人工计算 dd 偏移 | FIT 内声明，U-Boot 自动搬运 |
| 架构校验 | 无 | FIT 内 `arch = "riscv"` |
| 完整性 | 无 | FIT 内可带 CRC32 |
| SD 卡布局依赖 | 依赖容量与分区表 | 只依赖 FAT 分区（标准） |
| 误操作风险 | 写死设备名 | 必须显式传 `SDCARD=` |

---

## 1. 前置准备

### 硬件

- VisionFive2 开发板（2GB / 4GB / 8GB 均可）
- microSD 卡（建议 8GB 以上，Class 10）
- USB-TTL 串口转接器（3.3V！**不要用 5V**）
- 杜邦线若干

### 串口接线

开发板的 GPIO 排针上有 UART 引脚，接到 USB-TTL：

| 开发板 | USB-TTL |
|---|---|
| GPIO 6 (UART0 TX) | RXD |
| GPIO 8 (UART0 RX) | TXD |
| GND | GND |

> ⚠️ **不要把 VCC 接上**。开发板自己供电（USB-C 或 DC 接口），
> 串口只接 TX/RX/GND 三根线。接错 VCC 可能损坏板子。

### 串口参数

```
波特率: 115200
数据位: 8
校验:   无
停止位: 1
流控:   无
```

Linux 下用 `minicom` 或 `screen`：

```bash
sudo screen /dev/ttyUSB0 115200
# 退出: Ctrl-A 然后 K
```

Windows 下用 PuTTY（Serial 模式）。

### 软件

```bash
# 交叉工具链
sudo apt install gcc-riscv64-unknown-elf

# 设备树工具（用于校验 FIT 镜像结构）
sudo apt install device-tree-compiler

# 注意：不需要 u-boot-tools！
# 本项目的 FIT 生成器是自己实现的（tools/image/fitgen.c）
```

---

## 2. 生成镜像

```bash
cd ecnu-oslab-2026
make CONFIG=riscv64-visionfive2-uboot image
```

输出：

```
fitgen: build/riscv64-visionfive2-uboot/kernel.bin -> build/riscv64-visionfive2-uboot/kernel.itb
fitgen:   内核大小 31399 字节, 加载地址 0x40200000, 入口 0x40200000
fitgen:   镜像总大小 32040 字节, 内核数据偏移 640
fitgen:   内核 CRC32 = 0x74c64df1

===== 开发板交付物已生成 =====
  FIT 镜像  : build/riscv64-visionfive2-uboot/kernel.itb
  磁盘镜像  : build/riscv64-visionfive2-uboot/disk.img
```

### 验证 FIT 镜像结构（建议每次都做）

FIT 本质是一个带 FDT 头的容器，所以可以直接用 `dtc` 解析它：

```bash
dtc -I dtb -O dts build/riscv64-visionfive2-uboot/kernel.itb
```

应当输出类似：

```
/dts-v1/;

/ {
	description = "ECNU OSLab 2026 (riscv64-visionfive2-uboot)";
	#address-cells = <0x01>;

	images {
		kernel {
			description = "OSLab kernel";
			data-size = <0x7aa7>;
			data-offset = <0x280>;
			data-position = <0x280>;
			type = "kernel";
			arch = "riscv";
			os = "linux";
			compression = "none";
			load = <0x40200000>;
			entry = <0x40200000>;
		};
	};

	configurations {
		default = "conf-1";
		conf-1 {
			description = "ECNU OSLab 2026 (riscv64-visionfive2-uboot)";
			kernel = "kernel";
		};
	};
};
```

**检查三个关键点**：

1. `arch = "riscv"` — 架构正确
2. `load` 与 `entry` 都是 `0x40200000` — 与 `platform/visionfive2/platform.h`
   的 `PLAT_KERNEL_BASE` 一致
3. `data-offset` 指向的位置放的就是内核二进制

第 3 点可以这样验证：

```bash
# 用 dtc 输出的 data-offset（这里假设是 0x280 = 640）
cmp <(dd if=build/riscv64-visionfive2-uboot/kernel.bin bs=1 2>/dev/null) \
    <(dd if=build/riscv64-visionfive2-uboot/kernel.itb bs=1 skip=640 \
        count=$(stat -c%s build/riscv64-visionfive2-uboot/kernel.bin) 2>/dev/null)
# 没有输出 = 完全一致
```

---

## 3. 准备 SD 卡

### 为什么需要两个文件

- `kernel.itb` — 内核（由 U-Boot 的 `bootm` 加载）
- `disk.img` — 文件系统镜像（内核作为块设备读取，里面是用户程序）

### 分区方案

最简单的方式：**单个 FAT32 分区，两个文件都放里面**。

```bash
# 1. 找到 SD 卡设备（务必确认！）
lsblk -d -o NAME,SIZE,TYPE,TRAN
# 找 TRAN=usb 的那个；如果是读卡器内置，可能显示 mmc

# 2. 假设是 /dev/sdb，先用 fdisk 建一个 FAT32 分区
sudo fdisk /dev/sdb
#   n -> p -> 1 -> 回车 -> 回车 -> t -> b (W95 FAT32) -> w

# 3. 格式化
sudo mkfs.vfat -F 32 /dev/sdb1

# 4. 挂载并拷贝
sudo mkdir -p /mnt/vf2
sudo mount /dev/sdb1 /mnt/vf2
sudo cp build/riscv64-visionfive2-uboot/kernel.itb /mnt/vf2/
sudo cp build/riscv64-visionfive2-uboot/disk.img   /mnt/vf2/
sync
sudo umount /mnt/vf2
```

> ⚠️ **`make card` 目标不会自动分区或写卡**，只会提示你手动操作。
> 这是刻意的：一个悄悄执行 `dd`/`mkfs` 的 make 目标，在设备名写错时
> 会清掉你的硬盘。磁盘操作必须由人明确执行。

### 关于启动模式

VisionFive2 的启动模式由板上的 QSPI Flash 决定：

- **1-bit QSPI Nor Flash 模式**（出厂默认）：从 Flash 里的 U-Boot 启动，
  然后 U-Boot 可以从 SD 卡读文件。**本指南用这个模式。**
- **SD 卡启动模式**：需要把 U-Boot SPL 也写到 SD 卡特定位置。
  本课程不需要。

切换方式：连续按两次板上的复位键（RESET）。

---

## 4. 在 U-Boot 中启动

### 建立串口连接并上电

```bash
sudo screen /dev/ttyUSB0 115200
```

上电后你会依次看到（约 10-15 秒）：

```
StarFive SPL ...
U-Boot SPL 2021.10 ...
...
U-Boot 2021.10 ...
StarFive #
```

进入 U-Boot 命令行后，**先用 `mmc` 命令确认能识别 SD 卡**：

```
StarFive # mmc dev 1
switch to partitions #0, OK
mmc1 is current device
```

> **为什么是 `mmc dev 1` 而不是 `0`**：
> JH7110 上 `mmc 0` 通常是 eMMC 或 SPI Flash，SD 卡在 `mmc 1`。
> 如果 `mmc dev 1` 报错，试 `mmc dev 0`。

### 加载并启动内核

```
StarFive # fatload mmc 1:1 ${kernel_addr_r} kernel.itb
StarFive # bootm ${kernel_addr_r}
```

**这两条命令做了什么**：

| 命令 | 作用 |
|---|---|
| `fatload mmc 1:1` | 从 mmc 设备 1 的第 1 个分区（FAT）读文件 |
| `${kernel_addr_r}` | U-Boot 预定义的环境变量，指向可用的内存地址 |
| `bootm` | **解析 FIT 头，校验架构，按声明的 load 地址搬运镜像，然后跳转** |

`bootm` 与 2025 版本用的 `go` 的本质区别：

```
go 0x40200000     →  无条件跳转到该地址。镜像在哪、对不对、是不是 RISC-V，
                     一概不管。地址写错就静默跑飞。

bootm <addr>      →  读 FDT 头 → 校验 magic → 检查 arch="riscv"
                     → 按 load 属性把 data 搬到 0x40200000
                     → 跳到 entry 属性指定的地址
```

### 期望输出

```
[oslab] kernel booting via SBI on visionfive2
[platform] visionfive2
[platform] DRAM  : 0x40000000 + 128 MB
[platform] kernel: 0x40200000
[platform] cpus  : 4 (boot hart = 1)
[platform] uart  : 0x10000000 (irq 32)
[pmem] 内核区: ...
...
[fs] 根目录内容:
       inum=2    test_1
```

### 想省掉每次手敲

把启动命令存进 U-Boot 环境变量：

```
StarFive # setenv bootcmd 'mmc dev 1; fatload mmc 1:1 ${kernel_addr_r} kernel.itb; bootm ${kernel_addr_r}'
StarFive # setenv bootdelay 1
StarFive # saveenv
```

之后上电就会自动启动（想进命令行就在 `bootdelay` 期间按键打断）。

---

## 5. 排查

### 完全没有输出

按顺序排除：

1. **串口本身通不通**：U-Boot 有输出吗？
   - 有 → 串口硬件没问题，问题在内核
   - 没有 → 检查接线（TX/RX 是否交叉）、波特率、USB-TTL 是否 3.3V
2. **文件真的在卡上吗**：
   ```
   StarFive # fatls mmc 1:1
   ```
3. **`fatload` 成功了吗**：命令会回显读取的字节数
4. **`bootm` 报什么错**：
   ```
   StarFive # bootm ${kernel_addr_r}
   Bad Magic Number          → FIT 头不对，文件可能传坏了
   Unsupported Architecture  → arch 不是 riscv
   ```

### 输出乱码

**最可能的原因：`PLAT_UART0_CLOCK` 不对**，导致波特率分频算错。

`drivers/serial/uart16550.c` 里：

```c
uint32 divisor = PLAT_UART0_CLOCK / (16 * 115200);
```

JH7110 的 UART 输入时钟是 24 MHz。如果写成别的值（比如从 QEMU
配置复制过来的 3.6864 MHz），实际波特率就会偏，输出乱码。

**验证方法**：换几个常见值试（24M、48M、3.6864M），
哪个输出正常就是哪个。

### 输出几行后卡死

**最可能的原因：页表映射不全。**

打开分页之后，访问任何地址都要经过页表。如果 `kvm_init()`
漏掉了某个区域（尤其是**设备 MMIO 区**），症状就是
"分页打开后串口突然不输出了"。

检查 `kernel/mm/vm.c` 的 `kvm_init()` 是否映射了：
1. 内核镜像区
2. 可分配物理内存区
3. **设备区（PLIC + UART）** ← 最容易忘
4. 该平台的存储控制器区

### 只启动了 3 个核（而不是 4 个）

**`PLAT_HART_MIN/MAX/NCPU` 不自洽。**

VisionFive2 上 hart 0 是监控核，内核用 hart 1..4。所以：

```c
#define PLAT_NCPU      4
#define PLAT_BOOT_HART 1
#define PLAT_HART_MIN  1
#define PLAT_HART_MAX  4      /* 不是 3！ */
```

如果写成 `PLAT_HART_MAX 3`，hart 4 会被判为非法而停掉。
编译期断言应该会拦住这个错误 —— 如果没拦住，说明断言也需要检查。

### 时钟走得比预期快/慢

**`PLAT_TIMER_INTERVAL` 与时钟频率不匹配。**

VisionFive2 的 `rdtime` 频率是 4 MHz，所以：

```c
#define PLAT_TIMER_INTERVAL 400000UL    /* 4e6 / 10 = 0.1 秒 */
```

如果直接抄 QEMU 的 `1000000`，实际是 0.25 秒一次中断，
所有基于 `timer_wait` 的等待都会变慢 2.5 倍。

---

## 6. 一个可以自己做的扩展

当前的 `drivers/block/sdhci.c` 是**框架实现**（lab-7 要求你完成）。
完成之后，你可以做一个更有意思的验证：

```bash
# 在 initcode 里调用 open() 打开磁盘上的 test_1，然后 exec 它
```

如果成功，你就实现了从 **SD 卡读取文件 → 加载 ELF → 执行** 的
完整闭环 —— 和一个真实操作系统启动用户程序的过程完全一样。

---

## 附：VisionFive2 关键参数速查

| 项目 | 值 | 说明 |
|---|---|---|
| DRAM 基址 | `0x40000000` | |
| OpenSBI 位置 | `0x40000000` | 占 2MB |
| 内核加载地址 | `0x40200000` | |
| UART0 | `0x10000000` | 16550 兼容 |
| UART0 中断号 | 32 | |
| UART 输入时钟 | 24 MHz | 用于算波特率 |
| PLIC | `0x0c000000` | SiFive PLIC |
| CLINT | `0x02000000` | |
| SD 控制器 (DW MSHC) | `0x16000000` | |
| 时钟频率 | 4 MHz | `rdtime` |
| CPU | 4×U74 + 1×S7 | S7 是监控核，hart 0 |
| 内核可用 hart | 1..4 | |
