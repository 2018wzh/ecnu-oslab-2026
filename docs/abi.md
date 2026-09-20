# lab-9 系统调用 ABI

RV64 S-mode / OpenSBI；ecall 的 a7 是完整调用号，a0..a5 是参数，a0 是返回值。
架构层只负责解码及返回寄存器；通用内核不读取 CSR。

从 lab-8 进入 lab-9 时必须重编全部用户程序：编号切换为下面的 1～22；
前序 print_str/print_int 和磁盘测试调用撤下。编号和全部声明由教师提供；
1～8 分派由教师提供，9～22 接线及函数主体由学生完成。

| 编号 | 调用 | 参数顺序 | 成功返回 |
| --- | --- | --- | --- |
| 1 | brk | 新堆顶，0 查询 | 堆顶 |
| 2 | mmap | 地址（0 首次适配）、字节长度 | 映射地址 |
| 3 | munmap | 地址、字节长度 | 0 |
| 4 | fork | 无 | 父得子 PID，子得 0 |
| 5 | wait | i32 状态地址，0 忽略 | 回收子 PID |
| 6 | exit | i32 状态 | 不返回 |
| 7 | sleep | tick 数 | 0 |
| 8 | getpid | 无 | PID |
| 9 | exec | path、argv | argc |
| 10 | open | path、mode | fd |
| 11 | close | fd | 0 |
| 12 | read | fd、len、addr | 字节数 |
| 13 | write | fd、len、addr | 字节数 |
| 14 | lseek | fd、u32 offset、flag | 新偏移 |
| 15 | dup | fd | 新 fd |
| 16 | fstat | fd、addr | 0 |
| 17 | get_dentries | fd、addr、buffer_len | 字节数 |
| 18 | mkdir | path | 0 |
| 19 | chdir | path | 0 |
| 20 | print_cwd | 无 | 0 |
| 21 | link | old_path、new_path | 0 |
| 22 | unlink | path | 0 |

read/write 失败返回 0，其他可失败调用返回 -1；exit 不返回。未知号 panic。
内存范围、页对齐及底层耗尽/非法用户复制 panic 继承前序章节；本章字符串格式超限则返回 -1。

open：OPEN_CREATE=1、OPEN_READ=2、OPEN_WRITE=4，可按位或。不增加截断任务。
lseek：u32 无符号偏移；LSEEK_SET=0、LSEEK_ADD=1、LSEEK_SUB=2，尽力而为移动；不额外规定越界策略。
get_dentries 容量/返回都按字节，是有效目录项批量传输，不是返回项数；失败 -1。
print_cwd 无缓冲参数，内核打印路径，成功 0、失败 -1。
inode_to_path 从缓冲区尾端逆向填充含 NUL 的路径，返回起始偏移；path+offset 才是字符串。

fstat 共 16 字节，小端，C/Rust repr(C) 布局相同：

| 字节偏移 | 字段 | 类型 |
| --- | --- | --- |
| 0 | type（Rust kind） | u16 |
| 2 | nlink | u16 |
| 4 | size | u32 |
| 8 | inode_num | u32 |
| 12 | offset | u32 |

全局 file 128 个，每进程 10 个；exec 最多 32 个参数，单参数含 NUL 最多 128 字节。
STR_MAXLEN=127 指内容最多 127 字节，另加 NUL；路径输入缓冲 128 字节。
复制到缓冲后必须验证其中存在 NUL，超长或缺终止符失败；路径组件继承 59 字节加 NUL。
Rust 用户虚拟地址保留 UserAddr/usize，经页表复制后才形成内核切片；禁止把用户整数直接转成引用。

fork 复制尚未推进的父 ecall frame；子经普通返回辅助写 0 并推进一次，父经用户 trap 推进一次。
exec 先完成新页表/frame 再替换旧资源，成功返回 argc。
用户 trap 在 dispatch 前保存原调用号，之后重新获取当前 frame（旧 frame 可能已释放）；
成功 exec 只写 a0=argc，保留新 PC。普通调用及 exec 失败才把旧 ecall PC 加 4，随后正常返回用户态。
C 使用 arch_syscall_finish，Rust 使用 HAL syscall::finish。
