// arch-api 中中断控制接口的实现: 操作 sstatus.SIE 位。
// generic kernel 通过 <kernel/arch.h> 调用, 不需要知道 sstatus 的存在。
#include <kernel/types.h>
#include <kernel/arch.h>
#include <asm/csr.h>

void arch_irq_enable(void)  { SET_CSR(sstatus, SSTATUS_SIE); }
void arch_irq_disable(void) { CLEAR_CSR(sstatus, SSTATUS_SIE); }

int arch_irq_is_enabled(void)
{
        return (READ_CSR(sstatus) & SSTATUS_SIE) != 0;
}

// 保存中断状态并关中断, 返回保存值。
// 恢复的是加锁前的状态, 不是无条件开中断, 否则嵌套临界区会被破坏。
uint64 arch_irq_save(void)
{
        uint64 old = READ_CSR(sstatus);
        arch_irq_disable();
        return old;
}

void arch_irq_restore(uint64 flags)
{
        if (flags & SSTATUS_SIE)
                arch_irq_enable();
        else
                arch_irq_disable();
}

uint64 arch_read_time(void)
{
        uint64 t;
        asm volatile("rdtime %0" : "=r"(t) : : "memory");
        return t;
}
