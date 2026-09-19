// 内核主入口 (generic kernel)。
// 本文件"什么都不知道": 不知道 CPU 是 RISC-V 还是别的、机器是 QEMU 还是 VF2,
// 只按顺序调用各子系统初始化。平台差异 (内存基址、CPU 数、设备地址) 全来自
// <platform.h> 由构建系统选择; 函数级差异 (定时器/块设备) 由链接不同驱动实现。
// 因此本文件在两个平台上完全相同 —— 这是抽象成功的标志。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/mm.h>
#include <kernel/block.h>
#include <kernel/fs.h>
#include <kernel/arch.h>
#include <kernel/proc.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <platform.h>

/* 板级信息打印与设备映射 (platform/ 层实现) */
void platform_init(void);

/* 各模块初始化 */
int  arch_cpu_id(void);
void proc_init(void);
void plic_init(void);
void plic_init_hart(void);

void sched_init_hart(void);

// smp: 启动其他 hart (由启动协议提供: SBI HSM / 单核直启)
void smp_start_others(void);

static volatile int g_started = 0;

int main(void)
{
        int cpuid = arch_cpu_id();

        if (arch_cpu_is_boot_hart()) {
                console_init();
                print_init();

                printf("\n");
                printf("====================================\n");
                printf("  ECNU OSLab 2026  (C)\n");
                printf("  平台: %s\n", PLAT_NAME);
                printf("====================================\n");
                printf("\n");

                platform_init();

                pmem_init();
                kvm_init();
                kvm_init_hart();
                printf("[main] 分页已开启, 当前运行在虚拟地址空间\n");

                /* 进程表 */
                proc_init();

                /* 为本 CPU 建立 idle 进程。
                 * 必须先于任何 sched_switch —— 否则 myproc() 返回 NULL,
                 * 而且"没有可运行进程"时无路可退。 */
                sched_init_hart();


                // 中断控制器: 全局部分只需一次
                block_init();


                // 文件系统: 依赖块设备就绪, 顺序不能颠倒。
                // 需要条件编译: fs_init 属 lab-8, 在 lab-4..7 文件系统源码未入
                // 构建, 直接调用会链接失败; 用阶段判断排除, 同一份 main.c 服务
                // 所有 lab 阶段。
                plic_init();

                {
                        static uint8 blkbuf[BLOCK_SIZE];
                        if (block_read(0, blkbuf, 1) == 0) {
                                printf("[main] 块设备自检: 块 0 前 16 字节 =");
                                for (int i = 0; i < 16; i++)
                                        printf(" %02x", blkbuf[i]);
                                printf("\n");
                        } else {
                                printf("[main] 块设备自检: 读块 0 失败!\n");
                        }
                }

                fs_init();
                if (fs_mounted())
                        fs_list_root();


                // 建立系统时钟的逻辑状态 (tick 计数), 全局的, 由启动核建一次。
                // 注意它和"装时钟中断"是两件事: 后者是每 hart 自己的。
                trap_arch_init();


        // 每 hart 装自己的时钟中断 (参数是绝对时刻, 不是间隔)。
        // tick 计数是全局的, 已由启动核 timer_create 建好。
                timer_create();
                timer_set_next(timer_interval());
                /* 通知其他 hart 可以开始初始化了 */
                __sync_synchronize();
                g_started = 1;

                /* 唤醒其他 hart (由启动协议实现: SBI HSM / 单核直启) */
                smp_start_others();
        } else {
                // 等启动核建好页表与内存分配器, 过早进入会踩未初始化状态。
                while (g_started == 0)
                        ;
                __sync_synchronize();

                // 激活内核页表 (satp 是每 hart 自己的寄存器, 从核必须自己开)
                kvm_init_hart();

                sched_init_hart();
                plic_init_hart();
                printf("[cpu %d] 启动完成\n", cpuid);
        }


        arch_irq_enable();

        if (arch_cpu_is_boot_hart()) {
                printf("[main] 准备进入用户态\n");
                proc_make_first();   /* 不返回 */
        }

        printf("[cpu %d] 进入调度循环\n", cpuid);

        /* 主动让出 CPU, 让调度器有机会切到 idle 或别的进程。
         * 之后每次时钟中断都会再次触发调度。 */
        for (;;) {
                printf("[cpu %d] 调度循环: 当前进程 %s, 让出 CPU\n",
                       cpuid, myproc() ? myproc()->name : "?");
                sched_switch();
        }
}
