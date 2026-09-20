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

位图最低位在前，bit=1 表示已分配。inode 位号就是 inode 编号，0 可以分配；data 位号加 data_first 得到文件系统内绝对块号。inode 槽长 64 字节，本章仅预留，不引入目录或 inode 内容解释。格式化后只有超级块前 48 字节非零，其余超级块、位图、inode 区和数据区均为零。

总长度为 5373079552 字节，即 10494296 个 512 字节扇区。工具通过设置文件长度创建稀疏镜像，不逐页写零；逻辑长度不等于宿主实际占用。默认以排他创建方式拒绝覆盖。显式 --force 使用截断模式重新打开，再写入超级块并扩展长度，旧元数据和旧数据不会残留。工具返回错误时不得使用不完整镜像。

C：`make disk`；重建：`make disk MKFS_FLAGS=--force`；另选路径：`make disk DISK=/path/new.img`。
Rust：`cargo xtask disk --config riscv64-qemu-virt`；重建追加 `--force`；按平台存于 target 对应配置目录。
普通 run/debug 都只打开已有镜像，不自动制盘或格式化。启动前须显式创建实验盘。不引入 FUSE。

VisionFive2 后端将文件系统块号转换为 `2097152 + block * 8`，并检查整个 8 扇区请求处于实验区。QEMU 镜像从文件起点对应块 0。布局与 U-Boot 说明见 [microSD 后端](visionfive2-sd.md)。
