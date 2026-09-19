// arch-api 中内存管理相关接口的实现。
// generic kernel 只调用 arch_mmu_activate()/arch_tlb_flush_all(),
// 不知道 satp 寄存器的存在。
#include <kernel/types.h>
#include <kernel/arch.h>
#include <kernel/mm.h>
#include <asm/csr.h>

// Sv39 的 satp 编码: MODE(4bit) | ASID(16bit) | PPN(44bit)
#define SATP_MODE_SV39 (8UL << 60)

// 参数是页表的物理地址; satp 低 44 位存的是 PPN (物理地址 >> 12)。
// 若误把物理地址直接写进去, 会在之后取指时触发缺页而静默卡死。
void arch_mmu_activate(uint64 root_pa)
{
        uint64 ppn = root_pa >> PGSHIFT;
        arch_write_satp(SATP_MODE_SV39 | (ppn & 0xFFFFFFFFFFFFFUL));
        // 切换 satp 后必须刷新 TLB, 否则 TLB 里残留旧映射, 会导致
        // 随机性的地址翻译错误。
        arch_tlb_flush_all();
        // 刷新屏障: 确保 satp 写入生效后再继续执行后续取指。
        arch_mb();
}

void arch_tlb_flush_all(void)
{
        asm volatile("sfence.vma zero, zero" ::: "memory");
}

void arch_tlb_flush_page(uint64 va)
{
        asm volatile("sfence.vma %0, zero" : : "r"(va) : "memory");
}

void arch_mb(void)  { asm volatile("fence" ::: "memory"); }
void arch_wmb(void) { asm volatile("fence w,w" ::: "memory"); }

void arch_icache_sync(void)
{
        asm volatile("fence.i" ::: "memory");
}
