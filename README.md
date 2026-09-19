# LAB-4: 第一个用户进程的诞生

## 1. 代码组织结构

```
ecnu-oslab-2026
├── Makefile / configs/ / mk/          构建系统
├── include/kernel/
│   ├── proc.h             进程控制块、进程状态、每 CPU 当前进程
│   ├── fs.h              (占位: 本阶段还没有文件抽象, 参见 lab-9)
│   └── ...                其它接口
├── include/uapi/syscall.h 系统调用号 —— 内核与用户程序共用同一份定义
├── kernel/
│   ├── proc/
│   │   ├── proc.c          进程表、分配/释放、sleep/wakeup、fork
│   │   ├── init.c          创建第一个用户进程 (proc_make_first)  (TODO)
│   │   ├── exec.c          把镜像装进用户地址空间 + 进入用户态
│   │   └── initcode_blob.c 把内嵌的 initcode 变成可链接符号
│   ├── syscall/
│   │   ├── syscall.c       从 trapframe 取参数并分发  (TODO)
│   │   └── sysfunc.c       各系统调用的实现
│   ├── fs/
│   │   └── console.c       控制台
│   ├── trap/
│   │   ├── trap.c          时钟/外部中断的 generic 处理
│   │   └── timer.c         系统时钟状态
│   └── ...                 init/ lib/ sync/ mm/
├── arch/riscv64/
│   ├── include/asm/
│   │   ├── context.h       内核上下文 (callee-saved 寄存器)
│   │   └── trapframe.h     用户上下文 (全部寄存器)
│   ├── trap/
│   │   ├── entry.S         trap 入口 / 返回用户态
│   │   ├── trampoline.S    U-mode ↔ S-mode 的换栈辅助
│   │   └── trap.c          用户态陷阱处理 (trap_user_handler)  (TODO)
│   ├── process/
│   │   ├── context.c       上下文的初始化辅助
│   │   └── switch.S        内核上下文切换 arch_context_switch
│   └── ...                 boot/ mm/ smp/
├── platform/               机器相关 (地址、中断号、CPU 拓扑)
├── drivers/                设备协议 (串口、PLIC、定时器)
└── user/                   用户态程序与库
    ├── initcode.c          第一个用户进程 (会被嵌进内核镜像)
    ├── syscall.c / help.c  用户态 syscall 封装
    └── user.ld             用户程序的链接脚本
```

**标记说明**

**TODO**: 你需要实现新功能 / 你需要完善旧功能

## 2. 需要你完成的部分

本阶段要做的是让内核"生"出第一个用户进程 proczero: 准备它的地址空间, 切换进
用户态, 并正确响应它发出的系统调用。需要实现四处。

### 2.1 proc_make_first: 创建第一个用户进程

`kernel/proc/init.c` 里的 `proc_make_first` 按顺序完成:

1. `proc_alloc()` 拿一个进程槽位, 分配内核栈
2. 在内核栈顶端放一个 trapframe
3. 用 `exec_load_flat()` 把内嵌的 initcode 装进用户地址空间
4. 检查入口地址与加载地址一致 (initcode 是扁平二进制, 用 PC 相对寻址)
5. 调用 `user_enter(p)`, 它不返回

`exec_load_flat()` 和 `user_enter()` 已经写好。注意要先建 trapframe 再加载镜像,
因为 `exec_load_flat` 会把入口地址写进 `p->tf`。另外在 `kernel/init/main.c` 的
初始化流程结束后, 需要调用 `proc_make_first()`。

### 2.2 进入用户态

proc_make_first 最后调用的 `user_enter` 让 proczero 进入用户态。RISC-V 没有
"跳到 U-mode"的指令, 只有 `sret` (从 trap 返回)。做法是伪造一个 trapframe:
把 `sepc` 设为程序入口, 把 `sp` 设为用户栈顶, 然后走一遍"从 trap 返回"的路径。
第一次进入用户态和系统调用返回用户态走的是同一段代码。

`sret` 之前必须显式设置 `sstatus`: SPP=0 (返回 U-mode)、SPIE=1 (返回后开中断)。
不能依赖硬件的自动行为 —— `sstatus.SPP` 反映的是"进入 trap 时"的特权级, 而不是
你想返回到哪。少了这两行, 最常见的症状是"内核静默卡死": CPU 以为要返回
S-mode, 继续用内核页表执行用户代码。

### 2.3 trap_user_handler: 用户态陷阱处理

`arch/riscv64/trap/trap.c` 里的 `trap_user_handler` 处理从用户态陷入的 trap。
`trap_kernel_handler` 已经写好, 是它的参照物 —— 两者结构几乎一样, 区别只有三点:

1. 发生 trap 时 `sepc` 指向哪里 (用户的 trap 需要正确保存返回地址)
2. 要处理 `ecall from U` (系统调用)
3. 处理完之后要把 trap 入口换回内核向量

### 2.4 trap_handle_syscall: 系统调用分发

`kernel/syscall/syscall.c` 里的 `trap_handle_syscall` 从 trapframe 取出系统
调用号和参数并分发。两件容易忽略的事:

1. **推进返回地址**: 系统调用陷入时 `sepc` 指向 `ecall` 那条指令本身, 不推进
   返回后会再次执行同一条 `ecall`, 现象是"系统调用无限重复"。返回时设置为
   PC+4。
2. **错误码区分"没实现"与"调用号不存在"**:

   | 情况 | 返回 |
   |---|---|
   | 号在 ABI 里有定义但本阶段没实现 (目前只有 `mmap = 9`) | `E_NOSYS` (`-38`) |
   | 号在 ABI 里根本不存在 | `E_BADARG` (`-1`) 并打印一行提示 |

   两者都是负数, 但排查方向相反: `-1` 要去查自己的参数/调用号, `-38` 说明内核
   确实还没实现这个功能。`docs/abi-spec.md` 有同一张表。

## 3. 系统调用: 一组约定

用户程序要输出一段文字, 它不能直接写 UART 寄存器 (那是内核的事), 必须请求内核
替它做。本阶段的用户进程只发一个最简单的请求: `SYS_HELLOWORLD` —— 内核收到后
打印固定字符串 `proczero: hello world`。

系统调用的本质是一组约定: 用户程序传入系统调用号指定调用类型, 内核根据调用号
进入不同的响应分支, 读取其他参数、提供系统服务、返回处理结果。用户程序和内核
通过 trap 机制进行通信, 实现跨特权级的"函数调用"。

几个关键点:

- 调用号与参数必须**从 trapframe 里取**, 因为那是用户寄存器的快照, 内核拿不到
  "用户此刻的寄存器"。
- 返回值必须**写回 trapframe 的 a0**, 否则 `sret` 恢复寄存器时会用旧值覆盖掉
  返回值。
- `sepc` 必须 **+4**: `ecall` 是一条指令, 异常返回时 `sepc` 指向它自己, 不 +4
  就会无限重复执行同一条 `ecall`。

## 4. 测试

### 4.1 QEMU

```bash
make CONFIG=riscv64-qemu-virt-sbi run
```

实现正确时, 会在串口上看到用户程序打印的这段:

```
[main] 准备进入用户态
[init] 正在加载 initcode (... 字节)...
[exec] 已加载扁平镜像: ... 字节 @ 0x1000, 入口 0x1000
[init] 第一个用户进程已就绪 (pid 1)
[init] 切换到用户态...

proczero: hello world
proczero: hello world
```

**最后那段文字来自用户态** —— 这是本阶段唯一的验收标准。

### 4.2 VisionFive2

```bash
make CONFIG=riscv64-visionfive2-uboot image     # 生成 kernel.itb
```

把 `build/riscv64-visionfive2-uboot/kernel.itb` 复制到 SD 卡第一个分区, 在
U-Boot 里 `fatload mmc 1:1 ${kernel_addr_r} kernel.itb; bootm ${kernel_addr_r}`。
输出应与 QEMU 结构完全相同, 只有板级参数的数值不同。

## 尾声

到这里, 内核已经能创建并运行一个用户进程了。但系统里还没有调度器: 只有一个
用户进程, 它退出之后内核就地空转。`kernel/proc/proc.c` 里的 `proc_sleep` /
`proc_wakeup` / `proc_copy` (以及 `kernel/sched/`) 会在后面的阶段加入, 那时才
会有"多进程"。

---

## 7. 进阶目标（可选）

下面三条**不属于基本验收**：默认流程与本分支 README 的期望输出都不依赖它们。
它们的作用是把这一阶段的内核"做完整一点"——每条都只用到**本章已经给出的东西**，
不碰后面阶段的文件。每条写了"为什么值得做""思路（只说做法，不写代码）""怎么算做到"。

> 动手前先建自己的分支（例如 `git checkout -b my-advanced`）。
> **别破坏默认输出**：进阶改动如果改变了默认运行结果，后面几章的对照实验就失效了；
> 需要改默认行为时，用一个新的 `configs/` 配置或一个运行期开关把它隔开。

---

### 7.1 从"扁平映像"到 ELF 映像

**为什么值得做**：现在第一个用户程序是 `objcopy -O binary` 出来的**扁平映像**：
内核把它整段拷到固定地址就开始跑，没有"段"的概念。真实的用户程序是 ELF——
内核必须按 program header 逐段映射、按段设权限、把 `memsz > filesz` 的部分清零。
lab-9 会从**磁盘**加载 ELF；这一章可以先把"解析 ELF"单独练一遍，
数据来源是**内嵌的数组**（不需要文件系统）。

**思路**：
- 构建侧：把内嵌对象从"扁平二进制"换成**保留 ELF**（仍然用同一套嵌入机制）。
  注意现在链接用户程序用了一个"段不要按页对齐"的选项，换成 ELF 之后要重新想清楚
- 内核侧：写"内存里 ELF 的解析与校验"：魔数/类别/字节序/机器码、逐个 program header、
  虚拟地址必须落在用户地址范围内、按段标志给出读/写/执行权限、
  `memsz > filesz` 的部分清零
- 用"分配并映射用户页"的既有接口把每个段装进去
- **所有偏移与长度都来自外来数据**，每一处都要边界检查（这是 lab-9 加载磁盘 ELF 的预演）

**怎么算做到**：换成 ELF 映像后 initcode 照常跑通；
故意改坏一个段的虚拟地址或大小 → 被明确拒绝并给出原因，而不是跑飞。

**涉及**：`kernel/proc/exec.c`、`mk/build.mk`（嵌入规则）、`user/arch/riscv64/user.ld.in`　**难度**：★★★

### 7.2 第一个内核执行流（内核线程的雏形）

**为什么值得做**："进程"现在只属于用户态，但真实内核里有只在内核里跑的执行流
（idle、后台刷盘、解压 initramfs）。这一章还没有调度器与上下文切换，
所以只能做**雏形**：一条有独立内核栈、不返回用户态的执行流。
把它做出来，lab-6 的"内核线程"就只差"能被调度"这一步。

**思路**：
- 复用"分配进程结构 + 分配内核栈"的既有接口，手工构造它的入口——
  注意它不是从陷阱返回用户态，而是**进入一个内核函数**
- 先让它跑完打印一行就停下（等待中断），`main` 里只是顺序调用
- 提前想清楚 lab-6 会用到的两件事：上下文保存在哪里、被切换回来时从哪里继续；
  另外"设置内核栈顶"那个接口是给**用户态陷入**用的，内核执行流之间切换不经过它

**怎么算做到**：能打印出这条执行流自己的内核栈地址区间；它执行期间"当前进程"指向它自己；
它不返回用户态，也不踩启动流程的栈。

**涉及**：`kernel/proc/init.c`、`kernel/proc/proc.c`、`arch/riscv64/process/context.c`　**难度**：★★☆

### 7.3 HHDM：把物理内存线性映射到高半区

**为什么值得做**：现在内核是**恒等映射**——虚拟地址等于物理地址，
所以"物理地址 0x80200123"与"虚拟地址 0x80200123"在代码里长得一模一样，
指针到底是哪种地址只能靠注释和记忆。真实内核把全部物理内存线性映射到一个高半区（HHDM），
于是"物理页"永远通过"物理地址 + 偏移"访问，谁是物理地址一眼可辨。

**思路**：
- 链接脚本改成把内核放到高半区；入口处先建**两份映射**（一份恒等、一份高半区），
  跳到高地址之后再撤掉恒等映射——这是经典的 higher-half 引导流程
- 定义偏移常量与两个转换函数（物理↔虚拟），并规定：凡是"物理页号/物理地址"，
  使用前必须转换
- 页表接口的语义要写清楚：页表里存的是**物理**地址，翻译接口返回的也是物理地址
- 这是本章最伤筋动骨的一条：会影响 lab-2 之后每一处指针运算。建议分三步走、每步都能跑：
  ① 内核镜像上高半区；② 全内存线性映射；③ 把分配器、设备映射、页表代码改成用转换函数

**怎么算做到**：内核跑在高半区（打印一个内核函数地址，高位全 1）；物理↔虚拟转换往返一致；
设备仍可访问（串口输出正常）；`git grep` 里不再有"裸物理地址直接当指针用"的地方（引导早期除外）。

**涉及**：`arch/riscv64/linker/kernel.ld.in`、`arch/riscv64/boot/entry.S`、`kernel/mm/vm.c`、`kernel/mm/pmem.c`　**难度**：★★★
