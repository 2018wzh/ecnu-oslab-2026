// 内核主入口 (generic kernel)。
// 本文件"什么都不知道": 不知道 CPU 是 RISC-V 还是别的、机器是 QEMU 还是 VF2,
// 只按顺序调用各子系统初始化。平台差异 (内存基址、CPU 数、设备地址) 全来自
// <platform.h> 由构建系统选择; 函数级差异 (定时器/块设备) 由链接不同驱动实现。
// 因此本文件在两个平台上完全相同 —— 这是抽象成功的标志。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/arch.h>
#include <kernel/mm.h>
#include <platform.h>

/* 板级信息打印与设备映射 (platform/ 层实现) */
void platform_init(void);

int main(void)
{
        // ===== lab-9 阶段 =====
        // 本阶段目标: 实现 fd 表与文件读写, 完成系统整合。

        console_init();
        print_init();

        printf("\n");
        printf("====================================\n");
        printf("  ECNU OSLab 2026  (C)\n");
        printf("  LAB-2: 内存管理初步\n");
        printf("====================================\n");
        printf("\n");

        platform_init();


                // 物理内存分配器。必须先于任何需要分配页面的操作
                // (页表创建、进程创建都依赖它)。
        for (;;)
                ;
}
