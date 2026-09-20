#include <kernel/syscall.h>
#include <kernel/uvm.h>
#include <kernel/print.h>
#include <uapi/syscall.h>
// TODO(lab-5): 校验地址、字节长度（长度非零，均页对齐、无溢出；地址为零或位于 mmap 区），非法返回 -1，并调用 uvm_mmap。
long sys_mmap(const syscall_args_t *call)
{ (void)call; panic("TODO(lab-5): sys_mmap"); }
// TODO(lab-5): 校验非零页对齐长度、页对齐地址、无溢出及 mmap 区范围，非法 -1；解除后返回 0。
long sys_munmap(const syscall_args_t *call)
{ (void)call; panic("TODO(lab-5): sys_munmap"); }
// TODO(lab-5): 0 查询；非零须页对齐且在 [0x2000, MMAP_BEGIN]，否则 -1。
// 比较新旧堆顶，分别组合 heap_grow / heap_ungrow，不变不分配，成功返回新堆顶。
long sys_brk(const syscall_args_t *call)
{ (void)call; panic("TODO(lab-5): sys_brk"); }
/* 下面三个调用只服务于 lab-5 测试，lab-6 移除。
 * 地址必须经用户复制接口访问；非法复制输入可断言或 panic，不直接解引用。
 */
// TODO(lab-5): args[0] 是 i32 数组，args[1] 是元素数；复制后打印，成功返回 0。
long sys_test_copyin(const syscall_args_t *call)
{ (void)call; panic("TODO(lab-5): sys_test_copyin"); }
// TODO(lab-5): 将内核数组 {1,2,3,4,5} 复制到 args[0]，成功返回元素数 5。
long sys_test_copyout(const syscall_args_t *call)
{ (void)call; panic("TODO(lab-5): sys_test_copyout"); }
// TODO(lab-5): 从 args[0] 复制 NUL 字符串到学生安排的内核缓冲（遵守 maxlen）并打印，成功返回 0。
long sys_test_copyinstr(const syscall_args_t *call)
{ (void)call; panic("TODO(lab-5): sys_test_copyinstr"); }
