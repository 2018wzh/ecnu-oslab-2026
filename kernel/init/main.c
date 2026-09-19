// 内核主入口 (generic kernel)。
// 本文件"什么都不知道": 不知道 CPU 是 RISC-V 还是别的、机器是 QEMU 还是 VF2,
// 只按顺序调用各子系统初始化。平台差异 (内存基址、CPU 数、设备地址) 全来自
// <platform.h> 由构建系统选择; 函数级差异 (定时器/块设备) 由链接不同驱动实现。
// 因此本文件在两个平台上完全相同 —— 这是抽象成功的标志。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/mm.h>
#include <kernel/arch.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <platform.h>

/* 板级信息打印与设备映射 (platform/ 层实现) */
void platform_init(void);

/* 各模块初始化 */
int  arch_cpu_id(void);
void proc_init(void);

int main(void)
{
        console_init();
        print_init();

        printf("\n");
        printf("====================================\n");
        printf("  ECNU OSLab 2026  (C)\n");
        printf("  平台: %s\n", PLAT_NAME);
        printf("====================================\n");
        printf("\n");

        platform_init();

        /* 物理内存分配器 (必须先于任何分配页面的操作) */
        pmem_init();

        /* 建立内核页表并打开分页 */
        kvm_init();
        kvm_init_hart();
        printf("[main] 分页已开启, 当前运行在虚拟地址空间\n");

        /* 进程表: 为第一个用户进程准备槽位 */
        proc_init();


                // 为本 CPU 建 idle 进程并设为当前进程。必须先于任何 sched_switch,
                // 否则 myproc() 返回 NULL。
        arch_irq_enable();

        printf("[main] 准备进入用户态\n");

        /* 创建并进入第一个用户进程。它不会返回 ——
         * 返回路径是把 CPU 交给 U-mode 的那段代码。 */
        proc_make_first();

        printf("[main] proc_make_first() 意外返回了\n");
        for (;;)
                ;
}
