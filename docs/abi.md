# 用户 ABI（lab-5）

C/Rust 同用 RISC-V LP64D：a7 是完整调用号，a0～a5 是参数，a0 是返回值。架构层负责解码和返回封装；通用教师函数表负责分派，业务算法留作 TODO。只有系统调用的架构返回封装将 PC 加 4 一次；栈缺页成功后重试原指令。

| 编号 | 调用 | 成功结果 |
|---|---|---|
| 0 | hello() | 输出问候，返回 0 |
| 1 | test_copyin(address, count) | 读取并打印 count 个 i32，返回 0 |
| 2 | test_copyout(address) | 写入五个 i32：1、2、3、4、5，返回 5 |
| 3 | test_copyinstr(address) | 复制并打印 NUL 字符串，返回 0 |
| 4 | brk(top) | 新堆顶；0 查询 |
| 5 | mmap(address, byte_length) | 起始地址；address=0 首次适配 |
| 6 | munmap(address, byte_length) | 0 |

未知调用检查完整调用号，输出调用号与 pid 后 panic。本章没有 write，也没有用户 PROT 参数；匿名页固定 RWU，节点不含权限字段。

brk 非零请求须页对齐，范围 [0x2000, MMAP_BEGIN]，非法 -1。mmap/munmap 长度必须非零且页对齐，地址页对齐，区间不得溢出并位于 mmap 区；mmap 的零地址例外表示自动选址。系统调用层可检查的非法参数返回 -1；底层找不到空间、重叠、映射/解除失败以及物理页或节点耗尽 panic，不要求回滚和 -3。

用户复制支持非对齐、跨页和有界字符串，非法输入可断言或 panic。copyin 元素数没有“最多五个”限制，copyinstr 没有“128 字节”限制；学生安排内核缓冲。底层字符串最多复制 maxlen 字节，遇 NUL 提前停止，达到上限不强行补 NUL；Rust 目标切片长度就是 maxlen，调用者打印前应核对终止符或使用有界输出。

用户入口 0x1000，代码、数据及 BSS 的内存范围不超过一页，初始堆顶 0x2000。用户栈顶 TRAPFRAME=(1<<38)-8192，初始一页，预留4096页；MMAP_END=TRAPFRAME-4096*4096，MMAP_BEGIN=MMAP_END-16384*4096。只保存 ustack_npage，栈不收缩。两个特殊页均不设 U。

用户页表深复制不包含 frame/trampoline，不复制进程字段和 mmap 节点。销毁只针对已停止使用且独占的页表：frame 解除并释放到内核池，trampoline 只解除；普通用户页与页表页分别归还普通池和内核池。调用方清除失效 frame 指针，不得再次释放；高地址内核栈按前章约定另行管理。

三个临时 copy 服务仅用于 lab-5；后续移除及调用编号迁移须同步内核表、uapi、用户库、例程与文档，不保证与未整改的后续分支直接兼容。
