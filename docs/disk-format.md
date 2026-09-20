# 磁盘线格式

C/Rust 工具使用相同的固定偏移小端编码。每个字段是 u32，不能直接保存宿主结构体。

| 偏移（字节） | 字段 | 值 |
| --- | --- | --- |
| 0 | magic | 0x12341234 |
| 4 | block_size | 4096 |
| 8 | total_blocks | 1311787 |
| 12 | total_inodes | 65536 |
| 16 | inode_bitmap | 1 |
| 20 | inode_bitmap_blocks | 2 |
| 24 | inode_first | 3 |
| 28 | inode_blocks | 1024 |
| 32 | data_bitmap | 1027 |
| 36 | data_bitmap_blocks | 40 |
| 40 | data_first | 1067 |
| 44 | data_blocks | 1310720 |

令 B=4096、I=65536、D=1310720、S=64。inode_bitmap_blocks=ceil(I/(8B))；inode_first=1+inode_bitmap_blocks；inode_blocks=ceil(IS/B)；data_bitmap=inode_first+inode_blocks；data_bitmap_blocks=ceil(D/(8B))；data_first=data_bitmap+data_bitmap_blocks；total_blocks=data_first+D。各区连续且不重叠。

位图最低位在前，bit=1 表示已分配。inode 位号就是 inode 编号，0 可以分配；data 位号加 data_first 得到文件系统内绝对块号。inode 槽长 64 字节；lab-8 初始化根目录和两个普通文件，详见下节，其余未分配空间为零。

总长度为 5373079552 字节，即 10494296 个 512 字节扇区。工具通过设置文件长度创建稀疏镜像，不逐页写零；逻辑长度不等于宿主实际占用。默认以排他创建方式拒绝覆盖。显式 --force 使用截断模式重新打开，再写入超级块并扩展长度，旧元数据和旧数据不会残留。工具返回错误时不得使用不完整镜像。

C：`make disk`；重建：`make disk MKFS_FLAGS=--force`；另选路径：`make disk DISK=/path/new.img`。
Rust：`cargo xtask disk --config riscv64-qemu-virt`；重建追加 `--force`；按平台存于 target 对应配置目录。
普通 run/debug 都只打开已有镜像，不自动制盘或格式化。启动前须显式创建实验盘。不引入 FUSE。

VisionFive2 后端将文件系统块号转换为 `2097152 + block * 8`，并检查整个 8 扇区请求处于实验区。QEMU 镜像从文件起点对应块 0。布局与 U-Boot 说明见 [microSD 后端](visionfive2-sd.md)。


## lab-8 inode 与目录

根 inode 为 0；INVALID_INODE_NUM=0xFFFFFFFF。DATA/DIR/DEVICE=0/1/2；默认 major/minor=1/1。未分配数据索引为 0，绝不能把该规则用于 inode 编号。

每个 inode 64 字节：0/2/4/6 偏移为 LE u16 type/major/minor/nlink，8 为 LE u32 size，12～63 为 13 个 LE u32 索引（10 直接、2 一级、1 二级）。每个索引块含 1024 个 LE u32。内核 inode_rw/rw 须以相同偏移编解码，不能持久化宿主结构体。

目录只用 index[0]，一个块含 64 个 64 字节槽；0～59 为名称字节，60～63 为 LE u32 inode 编号。name[0]==0 是空槽；size 为有效槽字节总数，不能拿 size 当作扫描终点。路径名称复制至多 59 字节并补 NUL，超长截断。

初始 inode 0/1/2 分别是根、ABCD.txt、abcd.txt，nlink 均为 1，size 为 256/5200/13000。根的四项为 .、..、ABCD.txt、abcd.txt，编号 0/0/1/2。根使用 DATA_FIRST；大写文件使用后续 2 块，小写文件使用再后续 4 块，按整个文件偏移循环 A～Z / a～z。inode 位图首字节 0x07，data 位图首字节 0x7f。空目录槽名称清零、编号 0xFFFFFFFF；其他未分配空间为零。两种 mkfs 均显式 LE 编码，保留排他创建、显式 --force 和稀疏大盘。
