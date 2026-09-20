# LAB-1: 机器启动

## 1. 代码组织结构
```
ECNU-OSLAB-2026
├── LICENSE        开源协议  
├── Makefile       编译运行整个项目  
├── configs        平台与启动配置
├── mk             构建、运行与镜像生成规则
├── pictures       README使用的图片目录  
├── README.md      实验指导书  
├── arch/riscv64   RISC-V相关
│   ├── boot
│   │   ├── entry.S
│   │   ├── start.c (TODO)
│   │   └── sbi.c
│   ├── cpu.c
│   ├── include/asm    寄存器与固件接口
│   └── linker/kernel.lds.S  定义内核程序在链接时的布局
├── platform       QEMU与VisionFive2平台参数
├── drivers/serial
│   └── uart16550.c
├── include/kernel 通用内核接口声明
└── kernel         内核源码
    ├── lock
    │   └── spinlock.c (TODO)
    ├── lib
    │   ├── console.c
    │   └── print.c (TODO)
    └── main.c (TODO)
```
## 2. 实验核心目标

完成多核的机器启动, 进入main函数并输出启动信息 (下图为双核示意)  

![alt text](pictures/01.png)

QEMU使用双核，VisionFive2使用四核，每个CPU输出一条启动信息

## 3. 具体任务

### 3.1 机器启动本身

要想实现上述核心目标，仔细想想只需要完成两件事

1. 让内核在QEMU或VisionFive2上跑起来（分别为双核、四核启动）：**entry.S** 到 **start.c** 到 **main.c**  

2. 让内核向屏幕输出一些字符串，也就是实现C语言中经常调用的`printf()`

第一件事需要你研究一下xv6的启动流程，只需要看到进入 **main.c** 就够了

与xv6的启动流程相比，本实验有一个不同之处：进入S-mode之前的工作已经由OpenSBI完成了。在QEMU上，我们直接通过OpenSBI启动；在VisionFive2上，则由U-Boot配合OpenSBI完成启动。

接下来需要你完成的是：让主核做好初始化，再启动其他核，让它们进入main函数。这里要注意，主核不一定是hart 0，可以通过代码中提供的接口判断。内核使用从0开始的CPU编号，而VisionFive2的硬件hart编号是1～4，二者不要混淆。

第二件事需要你先阅读一下**drivers/serial/uart16550.c**，里面包括串口（最基本的字符输入输出设备）驱动

读完之后你需要通过`console_putc`调用串口输出完成**kernel/lib/print.c**中的函数，你可以参考xv6的实现，也可以自己去做

完成本章任务后，可以用以下命令构建和运行：

```bash
make CONFIG=riscv64-qemu-virt-sbi build
make CONFIG=riscv64-qemu-virt-sbi run
make CONFIG=riscv64-visionfive2-uboot build image
```

将`build/riscv64-visionfive2-uboot/kernel.itb`复制到开发板TF卡的FAT分区，在U-Boot中加载：

```text
fatload mmc 1:1 ${kernel_addr_r} kernel.itb
bootm ${kernel_addr_r}
```

### 3.2 printf面临的资源竞争问题

串口是一种设备资源, `printf()`利用它输出字符本质是在一段时间内持有这种资源

例如, 输出`"hello,world!"`其实是连续占用串口资源12次, 调用12次`console_putc()`

假设同时存在第二个`printf()`执行流要打印`"hello,os!"`, 它就会与执行流1形成竞争关系

两条执行流交错带来的输出可能包括:

```
# 混乱的情况
hellohello,,world!os!
hheelllloo,,wosrld!!
hhello,world!ello,os!
......
# 有序的情况
hello,world!hello,os!
hello,os!hello,world!
```

我们需要一种手段, 保证`printf()`过程中, UART资源始终只被一个执行流占有同时不可抢占

生活中的例子: 公共卫生间通过"门锁"来保证马桶这一资源在一段时间内只被一人独占

映射到操作系统, 最简单的"资源锁"就是“自旋锁”, 它的实现位于**kernel/lock/spinlock.c**

```
# 在printf中使用自旋锁的方法

spinlock_t lk;

# 锁的初始化
spinlock_init(&lk, "print_lk");

# 上锁
spinlock_acquire(&lk);

# 独占资源
console_putc('O');
console_putc('S');
......

# 解锁
spinlock_release(&lk);

```

自旋锁的可靠性依赖**开关中断**和**原子操作**这两个关键概念，你需要完全理解

- 上锁前关闭本CPU的中断，可以避免中断处理再次请求当前CPU已持有的锁；解锁后恢复之前的中断状态

- 原子操作可以保证多CPU的情况下并行执行流不会同时上锁成功

完成上述工作后，你应当可以实现图片所示的效果 (在**main.c**的合适位置输出每个CPU的启动信息)  

## 4. 课后实验

这里有两个额外的实验帮助你理解锁的用处 

### 4.1 并行加法

在完成本章启动、打印和锁的任务后，临时替换主函数测试。这里保留`volatile sum++`演示竞争；`volatile`不能提供同步，并发访问sum在C语言中属于数据竞争，不能保证具体结果。

```c
#include <kernel/arch.h>
#include <kernel/print.h>
#include <platform.h>

    static int started = 0;

    static volatile int sum = 0;

    void main(void)
    {
        int cpuid = (int)arch_cpu_id();
        if(arch_is_boot_cpu()) {
            print_init();
            printf("cpu %d is booting!\n", cpuid);
            for(int cpu = 0; cpu < NCPU; cpu++)
                if(cpu != cpuid && arch_start_cpu(cpu) != 0)
                    panic("start_cpu failed");
            __atomic_store_n(&started, 1, __ATOMIC_RELEASE);
            for(int i = 0; i < 1000000; i++)
                sum++;
            printf("cpu %d report: sum = %d\n", cpuid, sum);
        } else {
            while(__atomic_load_n(&started, __ATOMIC_ACQUIRE) == 0);
            printf("cpu %d is booting!\n", cpuid);
            for(int i = 0; i < 1000000; i++)
                sum++;
            printf("cpu %d report: sum = %d\n", cpuid, sum);
        }   
        arch_park();    
    }  
```

在 **main.c** 中测试上述代码，所有CPU完成正常累加后的总数应为QEMU双核的`2000000`或VisionFive2四核的`4000000`；report输出的是各核读取时的值，打印顺序不代表完成顺序

但是未加锁的双核输出可以用下面的例子说明  

```
cpu 0 is booting!
cpu 1 is booting!
cpu 0 report: sum = 1128497
cpu 1 report: sum = 1143332
```

考虑如何使用锁进行修正，修正后的双核输出可能是这样的  

```
cpu 0 is booting!
cpu 1 is booting!
cpu 0 report: sum = 1996573
cpu 1 report: sum = 2000000
```

简单说明上锁和解锁的位置不同会有什么影响（tips: 锁的粒度粗细）

### 4.2 并行输出  

尝试去掉`printf`里的锁，参考4.1的实验思路，设计测试方法使得`printf`的输出出现交错的情况  

4.1和4.2的测试代码和实验结果可以附在你的README中, 但是不要体现在你的代码里

## 5. 关于代码仓库的维护

1. 每次实验需要在上次实验的基础上继续往下做，假设教师仓库已配置为`upstream`

    首次开始lab-1时，使用`git fetch upstream`和`git checkout -b lab-1 upstream/lab-1`获取并切换到实验分支；已有lab-1分支时直接切换

    完成lab-1并提交自己的实现后，若个人提交基于`upstream/lab-1`且尚无lab-2分支，可用以下命令进入下一次实验：

    ```bash
    git fetch upstream
    git checkout -b lab-2 lab-1
    git rebase --onto upstream/lab-2 upstream/lab-1
    ```

    解决冲突时保留自己的实现并接入新框架，你对lab-2的修改不会影响lab-1

    以此类推，当你从lab-1开始走到lab-9时，你会获得越来越完整和强大的内核  

2. 你的代码仓库应该由 **代码 + Markdown文档** 两部分构成  

    文档内容不做明确要求，你有很高的自由度决定写什么和写多少

    提供一些建议: 
    
    - 本次实验新增了哪些功能，实现了什么效果

    - 对本次实验中某个过程的理解和思考

    - 本次实验和之前的实验构成什么样的逻辑联系

    - 本次实验花费的时间, 你和队友的贡献分别是什么

    - 可以使用markdown的分层分点来增加条理性，便于别人阅读和抓住重点

    **总之，这是你的代码仓库，请对你自己的代码和文档负责**  
    
    **注意，代码是继承和连续发展的, 但文档不是，每次的文档都是全新一页**  

3. 提醒: 之所以要求大家维护代码仓库，是为了查看大家的提交记录

    所以请及时同步当天写的代码到线上仓库，不要攒到最后一口气提交，否则可能被误判为不当行为

## 进阶目标

### UEFI

内核开始运行之前，需要有人把它装入内存，并把控制权交给它。UEFI定义了一套固件与操作系统之间的接口，启动程序可以借助它读取文件、申请内存、获取内存布局。利用这些服务，我们可以尝试另一条装载内核的路径。

请你尝试编写一个UEFI启动程序，装载并进入内核。可以先以输出启动信息为目标，梳理固件把控制权交给内核的过程，再检查退出启动服务后，内核是否还依赖这些已经不能使用的服务。

### DTB 动态发现

本次实验把CPU数量、内存范围和串口地址等信息写在平台配置里，换一种硬件配置就可能需要修改代码。设备树可以把这些硬件信息组织起来，DTB就是它编译后的二进制形式。内核读取启动时传入的DTB，就有机会了解当前机器，而不必把所有参数写死。

请你尝试从DTB中读取CPU、内存和设备信息，先与现有平台配置比较，再改变QEMU的CPU数量或内存大小，观察内核能否识别变化。注意：冷启动传入的DTB与从核启动参数不是一回事，固件保留的内存也不能交给内核分配。

### BootInfo

如果内核既支持设备树，又支持其他启动方式，是否每个模块都要了解它们各自的信息格式？可以在启动代码和内核之间约定一个`BootInfo`结构，只记录内核需要的CPU、内存和设备信息。它不是另一种设备树格式，而是由我们自己设计的统一接口，启动代码负责把不同来源的信息整理进去。

请你先梳理当前启动路径需要传递哪些信息，再尝试用`BootInfo`把它们交给内核。观察内核是否还需要区分这些信息的来源，并检查启动时的临时数据不再使用后，`BootInfo`中的内容是否仍然有效。
