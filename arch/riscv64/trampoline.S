.section .trampoline,"ax"
.balign 4096
.global trampoline, user_vector, user_return
trampoline:
user_vector:
    csrrw a0, sscratch, a0
    sd ra, 8(a0)
    sd sp, 16(a0)
    sd gp, 24(a0)
    sd tp, 32(a0)
    sd t0, 40(a0)
    sd t1, 48(a0)
    sd t2, 56(a0)
    sd s0, 64(a0)
    sd s1, 72(a0)
    sd a1, 88(a0)
    sd a2, 96(a0)
    sd a3, 104(a0)
    sd a4, 112(a0)
    sd a5, 120(a0)
    sd a6, 128(a0)
    sd a7, 136(a0)
    sd s2, 144(a0)
    sd s3, 152(a0)
    sd s4, 160(a0)
    sd s5, 168(a0)
    sd s6, 176(a0)
    sd s7, 184(a0)
    sd s8, 192(a0)
    sd s9, 200(a0)
    sd s10, 208(a0)
    sd s11, 216(a0)
    sd t3, 224(a0)
    sd t4, 232(a0)
    sd t5, 240(a0)
    sd t6, 248(a0)
    csrr t0, sscratch
    sd t0, 80(a0)
    csrr t0, sepc
    sd t0, 256(a0)
    csrr t0, sstatus
    sd t0, 264(a0)
    ld t0, 272(a0)
    ld sp, 280(a0)
    ld t1, 288(a0)
    ld tp, 296(a0)
    csrw satp, t0
    sfence.vma zero, zero
    jr t1
# a0 = trapframe 虚拟地址，a1 = 用户 satp；调用点也必须在高地址 trampoline。
user_return:
    csrw satp, a1
    sfence.vma zero, zero
    csrw sscratch, a0
    ld ra, 8(a0)
    ld sp, 16(a0)
    ld gp, 24(a0)
    ld tp, 32(a0)
    ld t0, 40(a0)
    ld t1, 48(a0)
    ld t2, 56(a0)
    ld s0, 64(a0)
    ld s1, 72(a0)
    ld a1, 88(a0)
    ld a2, 96(a0)
    ld a3, 104(a0)
    ld a4, 112(a0)
    ld a5, 120(a0)
    ld a6, 128(a0)
    ld a7, 136(a0)
    ld s2, 144(a0)
    ld s3, 152(a0)
    ld s4, 160(a0)
    ld s5, 168(a0)
    ld s6, 176(a0)
    ld s7, 184(a0)
    ld s8, 192(a0)
    ld s9, 200(a0)
    ld s10, 208(a0)
    ld s11, 216(a0)
    ld t3, 224(a0)
    ld t4, 232(a0)
    ld t5, 240(a0)
    ld t6, 248(a0)
    ld a0, 80(a0)
    sret
