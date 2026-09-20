#include <kernel/syscall.h>
#include <kernel/print.h>
// TODO(lab-6): args[0] 为用户 NUL 字符串，经 lab-5 copy_str_from_user 复制并有界打印，成功返回 0。
// 非法复制沿用前章 panic；不得直接解引用用户地址。
long sys_print_str(const syscall_args_t *call) { (void)call; panic("TODO(lab-6): sys_print_str"); }
// TODO(lab-6): args[0] 按有符号 32 位整数打印，成功返回 0。
long sys_print_int(const syscall_args_t *call) { (void)call; panic("TODO(lab-6): sys_print_int"); }
