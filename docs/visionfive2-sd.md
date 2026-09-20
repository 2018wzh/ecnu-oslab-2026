# VisionFive2 microSD 教师后端

本后端仅面向 JH7110 SDIO1、SDHC/SDXC 基础 3.3V 模式，采用 1-bit 总线、最多 25MHz、IDMAC 和请求完成中断。未实现 UHS、热插拔和性能调优。当前仅完成构建与静态复核，硬件读写未验证。

## U-Boot 前置条件与接入

先用板卡适配的 U-Boot 从 microSD 完成一次 `mmc rescan`、`mmc info` 和启动文件读取，确认供电、引脚、时钟及复位已经工作。不同 U-Boot 的 MMC 设备索引可能不同，须用 `mmc list` 识别 SDIO1，不能把示例中的编号机械套到 eMMC。提供给控制器的 ciu 时钟须不超过 200MHz，卡和引脚保持 3.3V，禁止 UHS/DDR 和 1.8V 切换；进入内核前 U-Boot 的所有请求/DMA 已结束。

内核不继承 U-Boot 的描述符或请求对象，也不依赖其 RCA。启动时重置控制器/FIFO/IDMAC，降低分频进行 CMD0、CMD8、ACMD41、CMD2、CMD3、CMD9、CMD7 枚举，检查 OCR.CCS、CSD v2 和容量，再建立 1-bit 基础传输状态。复用外部时钟、引脚和电源配置，不在本章重做板级初始化。分频 255 用于枚举，分频 4 用于传输。

启动步骤：先在宿主执行本仓库的 `image` 命令生成 kernel.itb，将它与匹配的设备树放在启动分区。U-Boot 中选择正确的 microSD 设备并 rescan，通过启动分区的 `fatload`/`ext4load` 将 FIT 和设备树读到互不重叠且不覆盖内核装载区的 DRAM 地址，再执行该板卡已有的 `bootm` 流程。具体装载地址及 FIT/DTB 参数沿用本仓库 docs 中的启动说明；不要通过这一步写实验区。

学生负责在内核页表映射 SDIO1 `0x16020000` 的 64KiB 和 CCACHE `0x02010000` 的 16KiB，均为内核 RW、无 U；初始化块设备后再使能 PLIC 中断 75，并在 claim 分支调用统一块中断入口，最后 complete。固件须允许 S-mode 访问这两个寄存器区。

## 卡布局与镜像写入

启动分区和实验区必须独立。实验区起始扇区固定为 2097152（1GiB），至少 10494296 扇区，排他结束扇区为 12591448；建议专用容量足够的 SDHC/SDXC 卡。分区表中的启动、根文件系统、备份 GPT 等结构不得落入该半开区间。基本内核不解析 MBR/GPT，不能依靠现有分区表自动避让。

在宿主分区工具中明确创建上述范围的实验分区，保留位于前 1GiB 内的启动分区和板卡固件区域，确保卡尾备份 GPT 位于实验区之外。先检查卡实际容量和完整布局；内核还会根据 CSD 检查卡容量。可以把分区做得更大，但后端只开放上述最小实验范围。

宿主生成全新 disk.img 后，以下是写入实验区的命令模板，仅供操作者核对设备与布局后手动使用；本轮没有执行实卡写入：

```sh
# CARD 必须指向已经核对布局、实验分区已卸载的整张卡。
# disk.img 是本章新建的 5373079552 字节逻辑镜像。
dd if=disk.img of="$CARD" bs=512 seek=2097152 count=10494296 conv=notrunc,fsync status=progress
```

不要使用 `conv=sparse` 写真实块设备：镜像中的空洞也必须写零，否则旧实验数据会残留。也可向恰好从指定扇区开始的实验分区设备写入镜像，此时不再加 seek。不要将镜像写到整卡偏移 0。以上布局说明不执行或授权额外的实卡操作。

## DMA 与完成契约

统一接口一次传输 4096 字节，即 CMD18/CMD25 的 8 个 512 字节扇区，使用自动 CMD12 停止多块传输。提交前检查范围，不将用户地址或高虚拟栈地址直接用于 IDMAC。

控制器 HCON 的地址宽度位决定 32/64 位 IDMAC 描述符格式和状态寄存器位置。描述符使用独立 64 字节对齐存储，数据使用独占完整页，均驻留恒等映射 DRAM。描述符与普通驱动状态分开，Rust 不对设备拥有的整个描述符区域建立可变引用。

SDIO 非一致 DMA 使用 JH7110 的 SiFive CCACHE FLUSH64：向基址+0x200 写入物理 cache line 地址，逐个清理并失效 64 字节行，前后 fence 排序。提交前同步数据页及描述符，再发布寄存器和 OWN；完成后先确认命令完成、数据结束、自动停止、IDMAC 完成及 DATA_BUSY 清除，再停 DMA、同步并检查描述符 OWN/CES，最后同步数据页并归还 CPU。普通内存屏障只保证顺序，不等于缓存刷新。物理页与描述符不能共享可由其他执行流修改的 cache line。

请求期间由块条件锁序列化 SD 后端。等待者睡眠时释放条件锁；数据页、描述符和请求状态留在原处。中断保存分次到达的完成位，直到完整完成才唤醒。错误中断或无法确认设备已停止时采用简化的致命失败契约，停止当前内核流程并保留 DMA 内存，不在设备所有权不明时归还页面；这不是通用错误恢复实现。设备完全不产生中断时尚无 watchdog，本章不增加超时回收任务。

## 实现依据

地址、中断、FIFO 深度和 CCACHE 地址核对 [Linux v6.16 JH7110 设备树](https://github.com/torvalds/linux/blob/v6.16/arch/riscv/boot/dts/starfive/jh7110.dtsi)。

DW-MSHC 寄存器、HCON 宽度位、IDMAC 描述符和完成位核对 [dw_mmc.h](https://github.com/torvalds/linux/blob/v6.16/drivers/mmc/host/dw_mmc.h) 与 [dw_mmc.c](https://github.com/torvalds/linux/blob/v6.16/drivers/mmc/host/dw_mmc.c)。

物理 cache line 操作核对 [SiFive CCACHE 非一致 DMA 维护](https://github.com/torvalds/linux/blob/v6.16/drivers/cache/sifive_ccache.c)。上述是接口与硬件契约依据，本仓库提供精简教学实现，不能据此推定某块卡或某版固件已验证。
