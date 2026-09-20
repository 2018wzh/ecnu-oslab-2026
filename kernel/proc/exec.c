#include <kernel/elf.h>
#include <kernel/print.h>
// TODO(lab-9): 先创建独立新页表和 frame，映射前序 trampoline/frame；读取 ELF header。
// 持 inode 锁调用教师 prepare_heap（含所有段的遍历），释放 inode，再调用 prepare_stack。
// 失败销毁新资源；成功才替换旧页表/frame，销毁旧地址空间和 mmap 链。
// 更新 heap_top、ustack_npage=1、mmap 空、name；保留 pid/parent/内核栈/files/cwd。
// 架构 frame 辅助设置新入口 PC/SP、argc/argv；成功返回 argc，失败 -1/Err。
// 不直接进入用户态：由 syscall 的返回辅助区分 exec 成功，随后走正常用户 trap 返回路径。
int proc_exec(const char *path, const char *const argv[], size_t argc)
{ (void)path; (void)argv; (void)argc; panic("TODO(lab-9): proc_exec"); }
