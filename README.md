# LAB-4: 第一个用户进程的诞生

**前言**

LAB-1到LAB-3属于第一阶段: **基础设施建设**

从LAB-4开始, OS内核的构建进入第二阶段: **用户进程管理**

到目前为止, OS内核只是一个能够掌控硬件的高权限(S-mode)程序

然而, 内核最核心的作用其实是为低权限的用户进程提供安全、共享、便捷的系统服务

引入进程模块并不是一件简单的事情, 你是否也感到无从下手呢?

按照"从简单到复杂"的基本原则, 我们先来研究"第一个用户进程是怎么一步步诞生的"

本次实验的**核心目标**: 用户进程向内核发出一个syscall, 内核收到后输出`proczero: hello world!\n`进行响应

## 代码组织结构

```
ECNU-OSLAB-2026
├── pictures       README使用的图片目录 (CHANGE, 日常更新)
├── README.md      实验指导书 (CHANGE, 日常更新)
├── arch/riscv64
│   ├── linker/kernel.lds.S (CHANGE, 支持trampoline)
│   ├── switch.S (NEW, 上下文切换)
│   ├── trampoline.S (NEW, 用户态进入与返回)
│   ├── syscall.c (TODO, 返回用户态前的准备)
│   └── user_image.S (NEW, 嵌入用户镜像)
├── include
│   ├── kernel
│   │   ├── proc.h (NEW, 进程结构与接口)
│   │   ├── context.h (NEW)
│   │   └── syscall.h (NEW)
│   └── uapi/syscall.h (NEW, 系统调用号)
├── kernel
│   ├── mem/kvm.c (TODO, 增加trampoline与KSTACK(0)映射)
│   ├── trap/user.c (TODO, 用户态陷阱处理)
│   ├── proc/proc.c (TODO, 进程管理核心逻辑)
│   └── main.c (TODO, 创建第一个用户进程)
└── user
    ├── init.c (NEW)
    ├── sys.h (NEW)
    ├── syscall.c (NEW)
    └── arch/riscv64
        ├── entry.S (NEW)
        ├── syscall_arch.h (NEW)
        └── user.ld (NEW, 用户程序布局)
```

**标记说明**

**NEW**: 新增源文件, 直接拷贝即可, 无需修改

**CHANGE**: 旧的源文件发生了更新, 直接拷贝即可, 无需修改

**TODO**: 你需要实现新功能 / 你需要完善旧功能

## 进程的定义和地址空间布局

### 用户进程诞生之前

进程指的是一个具备运行状态的程序(在Linux环境下通常以ELF文件存储)

也就是说进程由静态的可执行文件 + 动态的执行状态构成 (寄存器上下文、内存占用等)

在某种意义上, OS内核本身也可以视为一种进程 (它符合进程的定义, 虽然一般不叫它进程)

用户进程诞生之前, OS内核基于启动时使用的函数栈`boot_stacks`+**kernel/mem/kvm.c**中定义的`kernel_pgtbl`运行

它的主逻辑位于**kernel/main.c**, 进行了一系列系统资源初始化, 随后执行等待循环, 期间穿插trap的中断异常处理过程

这就是用户进程诞生之前OS内核的状态

### 用户进程的定义

这启发我们, 用户进程也至少要有**用户栈 + 用户页表**来支持它的运行

除此之外, 用户进程还需要记录哪些信息呢?

- 用于动态分配内存的空间——**用户堆**

- 用户进程陷入内核后, 需要有临时的函数执行空间——**内核栈**

- 用户进程陷入内核时, 必须保存用户执行流的上下文——**trapframe**

- 用户进程在内核中发生切换, 必须保存内核执行流的上下文——**context**

- 用户进程应该有一个自己的代号——**pid**

```c
// 进程
typedef struct proc
{
    int pid; // 标识符
    proc_state_t state; // 运行状态

    pgtbl_t pgtbl;       // 用户态页表
    uint64 heap_top;     // 用户堆顶(以字节为单位)
    uint64 ustack_npage; // 用户栈占用的页面数量
    user_frame_t *frame;     // 用户态内核态切换时的运行环境暂存空间

    uint64 kstack;       // 内核栈的虚拟地址
    context_t context;       // 内核态进程上下文
} proc_t;
```

### 用户进程的地址空间

![pic](./pictures/01.png)

图片示意了用户页表定义的用户进程地址空间 + 内核页表定义的内核程序地址空间

图中的堆和栈箭头表示后续增长方向, 本章只分配一页用户栈, 暂不分配堆页。两平台的内存和设备地址仍按平台配置确定

为了让用户程序顺利陷入内核执行, 内核页表在初始化时需要多映射以下两个部分

- **trampoline**: 定义在**arch/riscv64/trampoline.S**, 包含U-mode进入S-mode和返回的代码逻辑, 在两张页表中映射到相同的虚拟地址和物理页

- **kstack**: 宏`KSTACK(procid)` 定义了各个进程的内核栈地址空间, 目前只需映射proczero (procid = 0)

除此之外, 主核的`main`在完成初始化工作后会调用`proc_make_first`来准备proczero的初始状态。QEMU使用双核, VisionFive2使用四核, 但本章都只让主核进入这一个用户进程

proczero的初始化流程如下:

- 设置pid和state、从内核池申请并清零trapframe的物理页、通过`proc_pgtbl_init`申请用户页表(顺便完成trampoline和trapframe的映射, 不设置U权限)

- 从普通池申请ustack的物理页并映射为RWU、设置ustack_npage为1和heap_top为0x2000

- 为用户镜像(code + data)从普通池申请一个物理页、先清零再复制镜像、映射到0x1000并设置RWXU权限

- 设置frame中的regs.epc (返回后被置为PC)、regs.x[2] (用户sp)

- 设置内核相关的kstack、context.ra、context.sp字段 (内核栈页已由kvm_init分配映射, 不再重复分配)

- 关闭中断、设置当前进程, 通过arch_switch完成上下文切换

**值得注意的问题: 用户程序的镜像从哪来?**

启动时, 我们交给QEMU或开发板固件的是内核镜像

因此, 可以推断proczero对应的用户镜像一定会以某种方式嵌入内核

注意到**include/kernel/proc.h**中有这样的声明

```c
extern uint8 user_image_start[], user_image_end[];
```

它们标记了嵌入镜像的起止位置, **arch/riscv64/user_image.S**通过`.incbin`把镜像放入内核

这个镜像来自**user/init.c**。构建时先生成**init.elf**, 再用objcopy转换成不含ELF头的**init.bin**, 内核复制的是后者

修改**user/init.c**后, 按所用平台重新执行`make CONFIG=riscv64-qemu-virt-sbi build`或`make CONFIG=riscv64-visionfive2-uboot build`就能将新的用户镜像同步到内核

镜像的生成过程参见**mk/build.mk**的有关逻辑, 这里不做详细介绍。代码、数据和BSS共用一页, BSS不一定占用镜像文件中的字节, 因此复制前要清零整页, 并检查镜像长度不超过一页

## 特权级内上下文切换 (context)

让我们聚焦`proc_make_first`函数的最后一步: arch_switch(old_context, new_context)

`arch_switch`函数定义在**arch/riscv64/switch.S**中, 它的作用是将若干寄存器的值存入内存区域A, 再将内存区域B的值写入寄存器

获得寄存器的使用权意味着新的执行流开始工作, 失去寄存器的使用权意味着旧的执行流暂停执行

在`proc_make_first`里, 新的执行流是proczero, 旧的执行流是OS内核本身(entry.S->start.c->main.c->proc_make_first)

新执行流存储寄存器的内存区域是`proczero.context`, 旧的执行流呢?

注意到旧的执行流数量和CPU数量相等, 它们各自使用`boot_stacks`中的一段栈空间

因此, 我们用`boot_context[NCPU]`分别保存各核启动执行流的context

另外, `current[NCPU]`记录当前CPU执行的是哪个用户进程, 通过`myproc()`暴露给外界

## 特权级间上下文切换 (trapframe)

书接上文, `proc_make_first`在执行`arch_switch`之前会将`proczero.context.ra`设置为`enter_user`

这意味着`arch_switch`之后, proczero的执行流启动, 起始位置是`enter_user`

让我们追随proczero的执行流, 将注意力从**kernel/proc/proc.c**转移到**kernel/trap/user.c**和**trampoline.S**

`enter_user`是进程从内核态进入用户态前的准备工作。它先关闭中断、填写frame的内核信息, 再调用**arch/riscv64/syscall.c**中的`arch_user_return`完成其余准备。这些准备包括:

- 在frame的kernel_satp、kernel_sp、kernel_entry和kernel_hart中保存内核页表、内核栈顶、陷阱处理入口和hart编号

- 将trampoline高地址处的`user_vector`设为S-mode的trap处理入口 (控制流处于内核态则使用`kernel_vector`作为trap处理入口)

- 将frame中保存的regs.epc写入sepc寄存器, 确保返回用户态后PC指针处于正确的位置

- 将U-mode设为S-mode的上一个状态 (proczero第一次进入用户态时上一个状态不是U-mode, 手动设置一下)

- 设置返回后的中断状态、同步指令缓存, 准备参数并调用trampoline高地址处的`user_return`

我们将**trampoline.S**中的`user_vector`和`user_return`作为一个整体来看待, 更好地理解这个过程

- 在proczero第一次调用`user_return`时, 将用户页表中的`TRAPFRAME`虚拟地址保存到了`sscratch`寄存器

- `user_vector`先通过`sscratch`找到trapframe, 保存用户通用寄存器、sepc和sstatus

- `user_vector`随后恢复SP指针和hartid, 切换至内核页表, 调用`user_trap`

- `user_trap`的工作在下一节做具体介绍, trap处理完成后进入返回阶段(`enter_user`)

- `user_return`切换到用户页表, 恢复所有通用寄存器的值, 通过`sret`返回用户态

相比`kernel_vector`和`kernel_trap`组成的**A-B-A**结构

`user_vector`、`user_trap`、`enter_user`、`user_return`构成了更复杂的**A-B-C-D**结构

值得注意的是: proczero 是直接从**C-D**开始的; 当用户态发生trap后, 才会走完**A-B-C-D**的完整过程

## 用户态陷阱处理

让我们先总结一下目前提到的三类执行流:

- S-mode内核程序: entry.S->start.c->main.c->proc_make_first->执行流停滞

- 进程的S-mode内核执行流: proczero刚诞生时从内核中开始执行, 遇到trap后也在内核中处理

- U-mode用户程序: proczero中用户镜像的执行处于U-mode

进一步理解context和trapframe的区别:

- context是S-mode内核程序切换到进程的S-mode内核执行流需要用到的临时内存空间

- trapframe是U-mode用户程序切换到进程的S-mode内核执行流需要用到的临时空间

- context不涉及特权级切换, 需要保存的寄存器数量和其他信息更少

接下来我们讨论最后一个部分: `user_trap`

由于`user_trap`和`kernel_trap`都是在S-mode处理trap, 所以整体逻辑基本一样

主要区别在于:

- 进入`user_trap`后会重写trap入口, 将它设为`kernel_vector`

- `user_vector`已经将发生trap的PC值保存到frame中, `user_trap`处理时需要保留它, 确保正确返回用户态

- `user_trap`需要多处理一种特殊的异常——**系统调用(syscall)**

这是我们第一次正式介绍**系统调用**(最重要的异常), 它在RISC-V中对应U-mode发出的ecall (8号异常)

注意: 系统调用返回时应该设置 PC=PC+4, 跳过本次ecall指令。`arch_syscall_return`会写入返回值并推进epc, 不要再重复加4。时钟和串口中断不推进epc

系统调用是用户程序请求内核服务的入口

**系统调用的本质是一组约定:**

- 用户程序传入系统调用号来指定系统调用类型, 内核通过`arch_syscall_decode`读取调用号和参数

- 内核程序根据系统调用号, 进入不同的响应分支: 读取其他参数, 提供系统服务, 返回处理结果

- 用户程序和内核程序通过trap机制进行通信, 实现跨特权级的"函数调用"

## 测试

**测试一: 系统调用**

```c
#include "sys.h"
void user_main(void)
{
    if (hello() != 0) for (;;) {}
    if (hello() != 0) for (;;) {}
    for (;;) {}
}
```

**user/init.c**里, 用户程序发出了两次系统调用, `user_trap`需要正确地响应它们

方法很简单: 对`SYS_hello`输出`proczero: hello world!\n`并返回0, 遇到未知调用号则返回-38

这个测试用于验证第一个用户进程是否能和内核交互并获得内核提供的服务

**测试二: 用户态的时钟中断和串口中断**

LAB-3中我们验证了内核态时钟中断和串口中断的响应

现在请你测试一下, 在用户态, 中断响应是否能正常工作

**尾声**

相信你可以感觉到, 用户进程的引入明显提高了OS内核的复杂度

进程模块和内存模块、陷阱模块有着密切的联系, 牵一发而动全身

因此, 我们做了细致的拆分, 让用户进程能力逐步变强, 数量由一到多

- 在LAB-5: 我们将赋予proczero更强大的内存管理能力, 并建立真正的系统调用体系

- 在LAB-6：我们将引入proczero的子子孙孙, 实现完整的进程生命周期管理和多进程调度

## 进阶目标

### 用户程序与内核跨语言组合

用户程序通过系统调用与内核交互, 两边不一定要使用同一种语言。只要对调用号、参数和返回值的传递方式有相同约定, C用户程序也可以向Rust内核请求服务, 反过来也是一样。

请你尝试将另一种语言编写的用户程序嵌入内核, 先比较两套仓库的用户镜像布局和系统调用接口, 再接入构建过程。观察两次hello能否返回0, 用户态时钟和串口中断是否仍正常。注意：嵌入的是平坦二进制, 入口、栈对齐和单页大小限制也需要保持一致。

### 内核线程

本次实验先通过context切换到进程的内核栈, 再进入用户态。如果新的执行流只执行内核函数, 就不需要准备返回用户态的过程, 这便是内核线程的一种起点。

请你尝试为一个简单的内核函数准备独立的栈和context, 切换过去并输出信息, 同时考虑这个函数返回后应当去哪里。观察切换前后的栈和寄存器是否正确, 多线程调度可以留到lab-6再继续尝试。

### HHDM

本次实验主要通过恒等映射访问物理内存, 虚拟地址与物理地址相同。HHDM (Higher Half Direct Map) 则把一段物理内存映射到高地址区域, 使二者相差一个固定偏移, 内核可以通过这段映射访问物理页。

请你先找出分配器和页表代码中依赖恒等映射的地方, 再选择一段内存尝试固定偏移映射。比较两种地址访问同一物理页的结果, 观察切换页表后是否仍能访问, 并检查这段映射是否与trampoline和内核栈重叠。设备地址和固件保留区需要单独考虑, 不能对所有地址都直接加上偏移。
