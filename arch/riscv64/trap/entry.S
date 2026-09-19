// trap 入口 (从用户态/内核态陷入)。
// 这是全内核最需要小心的汇编: 陷入时无可用栈, 须先"原子地"把 sp 换成内核栈。
// 进入用户态前内核把本进程内核栈顶写进 sscratch, 陷入时:
//   csrrw sp, sscratch, sp   # 一条指令同时换栈并暂存用户 sp
//   addi  sp, sp, -TF_SIZE   # 在栈顶之下分配 trapframe
// 不能用 per-CPU 数组 + tp 索引 (用户可改 tp), sscratch 用户态访问不到是唯一
// 可靠落脚点。"来自哪个特权级"由硬件维护的 sstatus.SPP 决定 (0=U,1=S),
// 不能靠"sscratch 是否为 0" —— 那是人会漏写的人为约定。
#include <asm/trapframe.h>
#define SSTATUS_SPP_BIT  8      /* 1 = 返回 S-mode, 0 = 返回 U-mode */
#define SSTATUS_SPIE_BIT 5      /* sret 后 sstatus.SIE 的值 */

.section .text
.align 4
.global trap_entry
trap_entry:
        // 来源: 用 sstatus.SPP 判断 (0=U,1=S), 硬件维护的权威依据。
        csrr    t0, sstatus
        andi    t0, t0, (1 << SSTATUS_SPP_BIT)
        bnez    t0, trap_kernel_entry

.Lfrom_user:
        call    arch_trap_unimplemented

.global trap_return_to_user
trap_return_to_user:
.Lreturn_to_user:
        // 接口约定: 参数 a0 = trapframe 指针。trapframe 永远位于内核栈顶端
        // (见 exec.c 的分配方式), 所以 sp=trapframe 等价于"sp=内核栈顶-tf大小",
        // 后续用固定偏移恢复。恢复后 sp 变用户 sp, 内核栈"消失", 下次陷入再由
        // sscratch 找回。必须显式接收 a0: 从 user_enter 首次进入时 sp 还指向
        // 内核调用栈, 假定 sp 会读到垃圾使 sepc=0,sret 跳到地址 0。
        call    arch_trap_unimplemented

.global trap_kernel_entry
trap_kernel_entry:
        addi    sp, sp, -TF_SIZE

        sd      ra, TF_RA(sp)
        sd      gp, TF_GP(sp)
        sd      tp, TF_TP(sp)
        sd      t0, TF_T0(sp)
        sd      t1, TF_T1(sp)
        sd      t2, TF_T2(sp)
        sd      s0, TF_S0(sp)
        sd      s1, TF_S1(sp)
        sd      a0, TF_A0(sp)
        sd      a1, TF_A1(sp)
        sd      a2, TF_A2(sp)
        sd      a3, TF_A3(sp)
        sd      a4, TF_A4(sp)
        sd      a5, TF_A5(sp)
        sd      a6, TF_A6(sp)
        sd      a7, TF_A7(sp)
        sd      s2, TF_S2(sp)
        sd      s3, TF_S3(sp)
        sd      s4, TF_S4(sp)
        sd      s5, TF_S5(sp)
        sd      s6, TF_S6(sp)
        sd      s7, TF_S7(sp)
        sd      s8, TF_S8(sp)
        sd      s9, TF_S9(sp)
        sd      s10, TF_S10(sp)
        sd      s11, TF_S11(sp)
        sd      t3, TF_T3(sp)
        sd      t4, TF_T4(sp)
        sd      t5, TF_T5(sp)
        sd      t6, TF_T6(sp)
        sd      sp, TF_SP(sp)

        csrr    t0, sepc
        sd      t0, TF_SEPC(sp)

        mv      a0, sp
        call    trap_kernel_handler

        /* 恢复现场 */
        ld      ra, TF_RA(sp)
        ld      gp, TF_GP(sp)
        ld      tp, TF_TP(sp)
        ld      t0, TF_T0(sp)
        ld      t1, TF_T1(sp)
        ld      t2, TF_T2(sp)
        ld      s0, TF_S0(sp)
        ld      s1, TF_S1(sp)
        ld      a0, TF_A0(sp)
        ld      a1, TF_A1(sp)
        ld      a2, TF_A2(sp)
        ld      a3, TF_A3(sp)
        ld      a4, TF_A4(sp)
        ld      a5, TF_A5(sp)
        ld      a6, TF_A6(sp)
        ld      a7, TF_A7(sp)
        ld      s2, TF_S2(sp)
        ld      s3, TF_S3(sp)
        ld      s4, TF_S4(sp)
        ld      s5, TF_S5(sp)
        ld      s6, TF_S6(sp)
        ld      s7, TF_S7(sp)
        ld      s8, TF_S8(sp)
        ld      s9, TF_S9(sp)
        ld      s10, TF_S10(sp)
        ld      s11, TF_S11(sp)
        ld      t3, TF_T3(sp)
        ld      t4, TF_T4(sp)
        ld      t5, TF_T5(sp)
        ld      t6, TF_T6(sp)

        addi    sp, sp, TF_SIZE
        sret
