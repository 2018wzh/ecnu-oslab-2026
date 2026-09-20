#include <kernel/proc.h>
#include <kernel/arch.h>
#include <kernel/print.h>
#include <platform.h>

// TODO(lab-3): 承接前序启动流程，接入共享 trap 初始化与每核 trap 初始化。
// TODO(lab-1): 主核初始化并启动其他核，以原子操作同步；每核打印一次。
// TODO(lab-5): 创建首进程前初始化 mmap 节点池。
// TODO(lab-6): 主核初始化进程表后完成内存、每核分页与中断初始化后调用 proc_make_first；所有核完成各自初始化与发布同步后进入调度器，永不返回。
void main(void)
{
    panic("TODO(lab-1): main");
}
