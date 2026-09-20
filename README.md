# LAB-9: 文件系统 之 文件管理与全系统整合

**前言**

恭喜你完成了前8次实验, 来到最后一个关卡

最后一个实验的内容比较多, 难度也比较大, 既是实验也是测验

- 测验你对整个系统的理解：内存、进程、文件系统、用户态程序等

- 测验你的编码与调试能力：文件操作、路径操作、ELF解析、系统调用等

不用担心, 助教会屏蔽大部分繁琐但不重要的工作, 并梳理实验脉络

这大概要花费你好几天时间, 现在就开始吧!

## 代码组织结构

```
ECNU-OSLAB-2026
├── include
│   ├── kernel
│   │   ├── file.h       文件接口 (NEW)
│   │   ├── elf.h        ELF接口 (NEW)
│   │   └── proc.h       进程结构 (CHANGE)
│   └── uapi
│       ├── fs.h         文件状态与目录项 (NEW)
│       └── syscall.h    系统调用编号 (CHANGE)
├── kernel
│   ├── lib
│   │   ├── console_input.c 行缓冲输入 (NEW)
│   │   └── elf.c        ELF装载辅助 (NEW)
│   ├── mem
│   │   ├── pmem.c       空闲页统计 (TODO)
│   │   └── uvm.c        用户堆权限 (TODO)
│   ├── trap
│   │   └── trap.c       串口输入处理 (CHANGE)
│   ├── proc
│   │   ├── files.c      文件与工作目录 (TODO)
│   │   ├── lifecycle.c  进程生命周期 (TODO)
│   │   ├── schedule.c   首进程初始化 (TODO)
│   │   └── exec.c       执行ELF文件 (TODO)
│   ├── syscall
│   │   ├── syscall.c   系统调用分派 (TODO)
│   │   └── file.c      文件系统调用 (TODO)
│   └── fs
│       ├── dentry.c    目录项操作 (TODO)
│       ├── path.c      路径操作 (TODO)
│       ├── file.c      文件操作 (TODO)
│       ├── device.c    设备文件 (TODO)
│       └── fs.c        文件系统初始化 (TODO)
├── tools
│   └── mkfs.c          导入用户程序 (CHANGE)
└── user
    ├── arch/riscv64
    │   ├── user.ld     初始用户程序链接脚本
    │   └── elf.ld      ELF程序链接脚本 (NEW)
    ├── init.c          启动测试程序 (CHANGE)
    ├── syscall.c       系统调用封装 (CHANGE)
    ├── help.c          公共库函数 (NEW)
    ├── help.h          类型与常量 (NEW)
    ├── sys.h           用户接口 (CHANGE)
    ├── test_1.c        测试点 (NEW)
    ├── test_2.c        测试点 (NEW)
    ├── test_3.c        测试点 (NEW)
    └── test_4.c        测试点 (NEW)
```

**标记说明**

**NEW**: 新增源文件, 直接拷贝即可, 无需修改

**CHANGE**: 旧的源文件发生了更新, 直接拷贝即可, 无需修改

**TODO**: 你需要实现新功能 / 你需要完善旧功能

## 第1步: 准备工作

1. 阅读**arch/riscv64/linker/kernel.lds.S**、**user/arch/riscv64/user.ld**和**user/arch/riscv64/elf.ld**, 比较内核、初始用户程序和ELF程序的链接布局

2. 在**kernel/mem/pmem.c**中实现`pmem_stat`, 用于获取当前的可用内存情况

3. 修改**kernel/mem/uvm.c**中的`uvm_heap_grow`, 根据输入的flags设置内存区域的权限, 使ELF各段可以使用各自的读写执行权限

4. 为了支持行缓冲的输入, 我们在**kernel/lib/console_input.c**中实现了行编辑和读取逻辑。请你阅读**kernel/trap/trap.c**, 理解它如何初始化输入缓冲区, 并将串口中断收到的字符交给`console_input`处理

5. 阅读 **user/** 中的各个源文件, 理解它们的组织架构和测试流程

6. 阅读**Makefile**和**tools/mkfs.c**, 理解各个测试程序如何写入**disk.img**

## 第2步: 完善文件系统 (fs)

**2.1 在lab-8中实现了dentry和path的部分函数，我们先进行补全**

```c
// in dentry.c
uint32 dentry_search_number(inode_t *ip, uint32 inode_num, char *name); // 基于inode_num搜索name
uint32 dentry_transmit(inode_t *ip, uint64 dst, uint32 len, bool is_user_dst); // 传输有效目录项
// in path.c
int inode_to_path(inode_t *ip, char *path, size_t len); // 与__path_to_inode相反的解析过程
inode_t* path_create(const char *path, uint16 type, uint16 major, uint16 minor); // 创建新的inode
int path_link(const char *old_path, const char *new_path); // 建立硬链接
int path_unlink(const char *path); // 解除硬链接
```

前两个函数相对简单, 你可以参考之前实现的`dentry_search`和`dentry_print`来做, 核心操作都是目录项遍历

`inode_to_path`相对复杂, 它的作用是获取某个inode(目录类型)的绝对路径, 也就是从某个节点开始回溯到树根节点

我们知道, 正向查找 (`path_to_inode`) 的理论依据是`dentry_search`操作进行文件名匹配

对应的, 逆向查找 (`inode_to_path`) 的理论依据是之前埋好的`..`目录项, 它对应的inode_num就是上级节点的inode_num

注意: 由于采用逆向填充方法, 所以缓冲区的使用也是从后往前的, 返回偏移量 (**path + offset**才是绝对路径字符串的起点)

下一个需要实现的函数是`path_create`, 它基于目标路径来创建新的inode, 包括inode的申请和目录项的创建等

在之前的假设中, 一个inode只对应一个绝对路径, 这可能带来一些不方便:

假设一个你经常使用的文件处于很深的绝对路径中, 打开它就变得麻烦了

为了解决这个问题, 我们引入了硬链接的方法: 一个inode可以对应多个绝对路径 

/AAA/BBB/CCC/DDD/hello.txt 对应 inode-23, /link.txt 也对应 inode-23

实现这一点只需要做两件事: (1) inode-23.nlink++ (2) 在根目录下增加一条dentry {link.txt, 23}

对应的, unlink操作也完成两件事: (1) inode.nlink-- (2) 删除一条dentry

它还需要考虑一个问题: inode的资源释放 (当nlink减到0, 意味着用户无法通过路径方法访问这个inode, 等最后一个引用释放后才能回收它)

资源释放的判断逻辑, 我们在`inode_put`中已经实现了, 这里不必显式执行`inode_delete`操作

理解这些部分后, 请你动手实现 `path_link` 与 `path_unlink`

**2.2 补全了dentry.c和path.c中的函数后, 我们正式引入文件的抽象**

你可能听过一句话: Linux秉持一切皆文件的设计哲学; 下面详细讨论"文件"的定义与实现方法

```c
typedef struct file {
    inode_t *inode;     // 对应的inode
    bool readable;      // 是否可读
    bool writable;      // 是否可写
    uint32 offset;      // 读/写指针的偏移量
    uint32 refs;        // 引用数 (file_table_lock保护)
} file_t;

file_t file_table[N_FILE]; // 文件资源池
spinlock_t file_table_lock; // 保护它的锁

```

除了inode指针, 文件还包括读写权限字段、读写指针偏移量字段、引用数字段

其中读写权限字段在文件开始时设置、偏移量字段在文件读写时设置、引用数字段在文件打开关闭复制时设置

值得注意的是, 不同于inode、buffer等全局共享资源; 对于进程来说, 文件提供了一种独占inode资源的假象

文件的读写权限在打开时设置。fork和dup会共享同一个file, 因此读写普通文件时, 应先取得inode锁, 等数据传输和offset更新完成后再解锁。移动偏移量和读取文件状态时也使用这把锁, 引用数则由全局的file_table_lock保护

我们可以梳理一下file与inode的区别: 

- file是由进程持有、可经fork/dup共享的、具备动态语义的、字段不做持久化存储的 数据集合管理者

- inode是全局共享的、记录文件数据和元信息的、部分字段进行持久化存储的 数据集合管理者

理解上述概念后, 请你实现一系列文件相关的函数

```c
// in file.c
void file_init(void); // 初始化file_table和锁
file_t* file_alloc(void); // 获取空闲file
file_t* file_open(const char *path, uint32 open_mode); // 打开文件(注意打开模式)
void file_close(file_t *file); // 关闭文件
long file_read(file_t* file, uint32 len, uint64 dst, bool is_user_dst); // 读取文件
long file_write(file_t* file, uint32 len, uint64 src, bool is_user_src); // 写入文件
long file_seek(file_t *file, uint32 lseek_offset, uint32 lseek_flag); // 读写指针移动
file_t* file_dup(file_t* file); // 复制文件使用权
int file_stat(file_t* file, file_stat_t *out); // 获取文件状态
```

在实现这些函数的过程中, 你应该注意到: `file_read` 和 `file_write` 需要用到switch-case做分类处理

file type 与 inode type 是匹配的, 分成: 数据文件(流式)、目录文件（结构化）、设备文件（特殊定义）

**2.3 数据文件和目录文件我们比较熟悉了, 下面重点介绍设备文件的情况**

在我们的设计中, 设备文件的分类只使用主设备号**inode->disk_info.major**, 次设备号都使用**default**

设备文件的核心特性是支持**读写**操作, 不同于另外两种文件的读写都是在磁盘上进行的

设备文件的读写操作是可以灵活定义的, 我们定义了六种设备文件:

- **/dev/stdin**: 标准输入 (行缓冲), 可读

- **/dev/stdout**: 标准输出, 可写

- **/dev/stderr**: 标准错误输出 (前置输出“ERROR”), 可写

- **/dev/zero**: 零文件 (可以读到任意多的零字节), 可读

- **/dev/null**: 黑洞文件 (读到的字节数永远是0, 写入多少字节都可以), 可读可写

- **/dev/gpt0**: 小彩蛋 (可以回答预设问题的笨蛋版本GPT), 可写

这六种设备文件的读写函数已经给出, 你需要实现下面的功能:

```c
// in device.c
int device_init(void); // 初始化device_table, 保证各个设备文件/dev/xxx在磁盘中存在
bool device_open_check(uint16 major, uint32 open_mode); // 检查设备文件是否存在及打开权限的合法性
long device_read(uint16 major, void *dst, size_t len); // 读接口
long device_write(uint16 major, const void *src, size_t len); // 写接口
```

注意: `device_init`和`file_init`应该在`fs_init`中被调用

## 第3步: 进程与文件系统 (proc/files.c、lifecycle.c、schedule.c)

完成文件系统的补全工作后, 我们进一步讨论进程模块与文件系统模块的协作

首先关注进程结构体中的打开文件表字段`files`和当前工作目录字段`cwd`

前者记录当前进程打开的文件指针, 后者记录了当前进程**站在**文件系统树中的哪个位置

我们需要修改哪些地方以支持这两个字段的生命周期呢?

- `proc_table_init`和`proc_slot_alloc`: 初始化资源(设为NULL)

- `proc_first_return`: 对于新生的proczero, 在文件系统初始化后调用`proc_files_init`, 设置files(依次打开stdin stdout stderr)和cwd(设为根目录)

- `proc_fork`: 对于其他进程, 调用`proc_files_clone`继承父进程的files和cwd(file_dup + inode_dup)

- `proc_free`: 关闭文件并释放cwd引用, 将相应字段设为NULL

关闭文件可能触发磁盘操作, 因此不能持着自旋锁关闭最后一个引用。请你按照`lifecycle.c`中的提示, 在进程锁内标记正在回收并取出files和cwd, 解锁后释放这些引用, 再重新加锁完成回收。分配和等待进程时也要检查这个标记, 避免其他核同时使用正在回收的槽位

和我们之前说的一样, 你会发现文件的生命周期与进程的生命周期是高度吻合的

支持cwd字段使得相对路径 (如./hello.txt or hello.txt or ../hello.txt) 变得可能 (区别于以`/`开头的绝对路径)

对应的, 请你修改路径解析函数 `__path_to_inode` 以支持基于相对路径搜索inode

## 第4步: 执行ELF文件 (proc/exec.c)

首先思考一下只有fork的OS内核是什么样的?

我们实现了user/init.c, 其他进程通过fork产生, 内容上和proczero没什么区别...

为了实现丰富多彩的用户软件, 只有fork是不够的, 我们还要有执行ELF文件的能力

ELF文件是编译链接的最终产物, OS内核读取和解析ELF文件, 在复制品的壳子上构建全新的血肉 (fork + exec)

ELF文件的组织结构通常是: [ELF_header | Program_header | seg-1 | seg-2 | ... | Section_header]

其中ELF_header描述了全局的情况 (类似Superblock), Program_header描述了各个segment的情况 (类似inode region)

接下来我们讨论如何实现非常重要的函数`proc_exec`

- step-0: 准备全新的pagetable和trapframe (为了防止中途崩溃, 我们不能直接修改旧的)

- step-1: 解析输入的文件路径, 获取ELF文件的inode

- step-2: 读取ELF_header, 其中**对Program_header的描述字段**和**ELF程序入口地址**是我们关心的

- step-3: 按照顺序读取需要载入内存的Segment, 填充到用户堆区域 (`prepare_heap`)

- step-4: 释放ELF的inode

- step-5: 处理输入的参数列表**argv**, 填充到用户栈区域 (`prepare_stack`)

- step-6: 新的地址空间构建完毕, 可以释放旧的资源了

- step-7: 设置trapframe的相关字段: 返回用户态的参数1(argc)、参数2(argv)、PC指针、SP指针

- step-8: 更新进程的相关字段: 页表、frame、heap_top、ustack_npage、mmap、name

助教将比较麻烦的`load_segment` `prepare_heap` `prepare_stack` 剥离出来并实现了, 你只需完成主线任务。成功时返回argc, 由系统调用的返回路径使用新的frame返回用户态

这个函数的复杂性大概是OS内核中最高的, 横跨进程、内存、文件系统三大核心模块, 需要你非常细心并充分理解每个步骤

## 第5步: 增加系统调用 (syscall)

为了便于在用户态进行系统测试, 你需要先实现一些新的系统调用, 主要是文件系统相关的 (9-22)

这是最新的系统调用表, 你需要参考它修改**kernel/syscall/syscall.c**与**kernel/syscall/file.c**中的缺漏

更多输入输出细节请参考**kernel/syscall/file.c**中各个系统调用函数的注释

```c
#define SYS_BRK 1               // 调整堆边界
#define SYS_MMAP 2              // 创建内存映射
#define SYS_MUNMAP 3            // 解除内存映射
#define SYS_FORK 4              // 进程复制
#define SYS_WAIT 5              // 等待子进程退出
#define SYS_EXIT 6              // 进程退出
#define SYS_SLEEP 7             // 进程睡眠一段时间
#define SYS_GETPID 8            // 获取当前进程的ID
#define SYS_EXEC 9              // 执行ELF文件
#define SYS_OPEN 10             // 打开文件
#define SYS_CLOSE 11            // 关闭文件
#define SYS_READ 12             // 读取文件
#define SYS_WRITE 13            // 写入文件
#define SYS_LSEEK 14            // 移动读写指针
#define SYS_DUP 15              // 复制文件权限
#define SYS_FSTAT 16            // 获取文件状态信息
#define SYS_GET_DENTRIES 17     // 获取目录下所有有效目录项
#define SYS_MKDIR 18            // 创建目录文件
#define SYS_CHDIR 19            // 切换工作目录
#define SYS_PRINT_CWD 20        // 打印工作目录的绝对路径
#define SYS_LINK 21             // 建立硬链接
#define SYS_UNLINK 22           // 解除硬链接
```

## 测试用例

实现`proc_exec`对系统调用的测试有很大帮助, 现在的测试流程是:

**user/init.c -> (fork + exec + wait) -> test_1 or test_2 or test_3 ...**

你只需要修改`user/init.c`中的**path**和**argv**参数即可启动不同的测试点

修改测试程序后, 需要重新生成磁盘映像, 才能运行更新后的ELF文件。QEMU使用`make CONFIG=riscv64-qemu-virt-sbi disk`, VisionFive2使用`make CONFIG=riscv64-visionfive2-uboot disk`。已有镜像不会自动覆盖, 可以通过`DISK=新文件路径`创建新的映像

助教准备了4个测试用例 (在`user`目录中), 请你依次执行, 参考输出见[测试1](pictures/01.png)、[测试2](pictures/02.png)、[测试3](pictures/03.png)、[测试4](pictures/04.png)

**尾声**

**历经9次实验, 我们终于完成了这个小型操作系统内核的全部工作, 祝贺!**

它由内核态程序、用户态程序、文件系统初始化程序、链接脚本四部分构成

**我们先来回顾一下9次实验的内容:**

- lab 1: 机器启动  
- lab 2: 内存管理初步  
- lab 3: 中断异常初步  
- lab 4: 第一个用户态进程的诞生
- lab 5: 系统调用流程建立+用户态虚拟内存管理
- lab 6: 从单进程走向多进程--进程调度与生命周期
- lab 7: 文件系统 之 磁盘管理  
- lab 8: 文件系统 之 数据组织与层次结构  
- lab 9: 文件系统 之 文件管理与全系统整合  

**这9次实验大致可以划分成3个阶段:**

- lab 1-3: 构建OS内核的基础设施, 例如串口、自旋锁、物理页、页表、中断异常等  
- lab 4-6: 按照“从一到多,从弱到强”的顺序构建进程模块, 并与内存模块、陷阱模块深度绑定  
- lab 7-9: 引入持久化存储概念, 自底向上构建文件系统模块, 并与进程模块深度绑定  

助教希望能帮你梳理系统构建的底层逻辑, 并带领你一步步来做; 但是受限于能力和精力, 难免有所缺漏, 请多包涵

经过这样的动手过程, 相信你对小型OS内核已经建立起了基本概念; 请记住, **这是起点而非终点**

**如果你想进一步完善和改进这个基础版本的OS内核, 我们提供了一些方向:**

- **内存管理**: (1) 实现更完善的缺页异常和写时拷贝机制; (2) 实现伙伴系统分配器
- **进程管理**: (1) 实现多级反馈调度算法来替换现有的轮询式调度; (2) 实现内核态线程机制
- **文件系统**: (1) 修改现有的mmap机制, 实现文件映射能力; (2) 实现基于FAT的文件系统, 构建VFS层
- **用户程序**: (1) 实现一个好用的shell程序; (2) 补充更多实用程序(如ls、cd、cat等)
- **硬件适配**: (1) 在QEMU和VisionFive2之外适配其他开发板; (2) 支持更多体系结构（如x86、ARM、LoongArch等）

**这个你亲手完成的小型OS内核, 将成为操作系统研究道路上的第一块砖, 支撑你走得更远!**

## 进阶目标

### PIE 程序加载

本次实验加载的ELF程序使用固定的链接地址。PIE程序可以装载到不同的地址, 但程序中的某些地址引用还需要根据装载位置进行调整, 这个过程称为重定位。

请你尝试支持一种不依赖动态链接器的简单PIE程序。可以先阅读它的程序头和重定位表, 选择少量重定位类型实现, 再将同一个程序加载到不同地址, 观察全局变量和指针访问是否正确。

### 其他文件系统

本次实验中的文件操作建立在我们自己的inode和目录布局上。其他文件系统可以采用不同的磁盘组织方式, 而用户仍希望通过open、read等接口访问文件。

请你选择另一种文件系统, 尝试从现有的文件和路径接口接入。可以先实现只读访问, 将相同内容的文件放入两种文件系统, 比较读取结果, 再考虑如何支持挂载和写入。

### procfs

我们已经用设备文件说明, 文件中的内容不一定来自磁盘。procfs将进程信息呈现为文件, 读取这些文件时, 内核根据当前的进程状态生成内容。

请你尝试提供只读的进程列表和单个进程的状态文件。可以先从PID、名称和运行状态入手, 再一边创建、退出进程, 一边读取这些文件, 观察是否会读到已经回收或被其他进程重新使用的槽位。

### newlib 移植

目前用户程序直接使用本实验提供的系统调用封装和辅助函数。newlib是一套面向嵌入式系统的C标准库, 将它接入内核后, 用户程序就有机会使用printf、malloc等熟悉的接口。

请你尝试运行一个静态链接newlib的C程序。可以从程序入口和read、write、brk等接口的适配入手, 注意标准库与本实验系统调用的返回值和数据布局差异, 再用打印、标准输入和内存申请释放的小程序检查结果。
