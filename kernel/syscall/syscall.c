// 系统调用分发 (generic kernel)。
// 系统调用是用户态与内核态之间唯一的合法通道。
// 调用链: 用户把调用号放 a7、参数放 a0-a5 后 ecall -> 硬件陷入 S-mode ->
// entry.S 保存现场并调 trap_user_handler -> trap.c 识别 ecall from U ->
// 本文件从 trapframe 取 a7/a0-a2 分发到各 sys_* 实现。
// 参数从 trapframe 取而非函数参数: 那些值已被保存在内核栈上的 trapframe 里,
// 所以 trapframe 字段布局必须与汇编严格一致。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/arch_trap.h>

/* trapframe 访问辅助 (由 arch 层提供) */
uint64 trapframe_get_reg(trapframe_t *tf, int idx);
void   trapframe_set_reg(trapframe_t *tf, int idx, uint64 v);

/* 在 trapframe 中的寄存器索引 (对应 trapframe.h 的 regs[] 数组下标)
 *   regs[0] = x1(ra), regs[1] = x2(sp), ... 索引 = 寄存器号 - 1 */
#define TF_IDX_A0 9    /* x10 = a0 */
#define TF_IDX_A1 10
#define TF_IDX_A2 11
#define TF_IDX_A3 12
#define TF_IDX_A4 13
#define TF_IDX_A5 14
#define TF_IDX_A7 16   /* x17 = a7: 系统调用号 */

/* 系统调用号定义在 UAPI 头文件中, 内核与用户程序共用同一份定义。
 * 这是分析报告强调的"单一事实来源": 不要分别手写内核分发表和用户封装。 */
#include <uapi/syscall.h>


// 各系统调用的实现 (lab-9 完善)
extern int64 sys_write(int fd, uint64 buf, uint64 n);
extern int64 sys_getpid(void);
extern int64 sys_open(uint64 path, uint64 flags, uint64 mode);
extern int64 sys_close(int fd);
extern int64 sys_lseek(int fd, uint64 offset, uint64 whence);
extern int64 sys_dup(int fd);
extern int64 sys_fstat(int fd);
extern int64 sys_get_dentries(int fd);
extern int64 sys_mkdir(uint64 path_user);
extern int64 sys_chdir(uint64 path_user);
extern int64 sys_print_cwd(void);
extern int64 sys_link(uint64 old_path_user, uint64 new_path_user);
extern int64 sys_unlink(uint64 path_user);

extern int64 sys_read(int fd, uint64 buf, uint64 n);
extern int64 sys_exit(int status);
extern int64 sys_fork(void);
extern int64 sys_exec(uint64 path, uint64 argv);
extern int64 sys_wait(uint64 status);
extern int64 sys_sleep(int n);
extern int64 sys_brk(uint64 new_brk);
extern int64 sys_mmap(uint64 len);
extern int64 sys_munmap(uint64 addr, uint64 len);


// 系统调用分发 (generic kernel)。
// 系统调用是用户态与内核态之间唯一的合法通道。
// 调用链: 用户把调用号放 a7、参数放 a0-a5 后 ecall -> 硬件陷入 S-mode ->
// entry.S 保存现场并调 trap_user_handler -> trap.c 识别 ecall from U ->
// 本文件从 trapframe 取 a7/a0-a2 分发到各 sys_* 实现。
// 参数从 trapframe 取而非函数参数: 那些值已被保存在内核栈上的 trapframe 里,
// 所以 trapframe 字段布局必须与汇编严格一致。
uint64 trap_handle_syscall(trapframe_t *tf)
{
}
