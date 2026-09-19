// hart 管理与 CPU 局部存储
#include <kernel/types.h>
#include <asm/csr.h>
#include <platform.h>

// 编译期校验: 这三个宏描述"内核能用几个核", 必须自洽, 否则少起来一个核。
STATIC_ASSERT(PLAT_HART_MAX - PLAT_HART_MIN + 1 == PLAT_NCPU,
              "platform.h 中 PLAT_HART_MAX/MIN 与 PLAT_NCPU 不一致");
STATIC_ASSERT(PLAT_BOOT_HART >= PLAT_HART_MIN && PLAT_BOOT_HART <= PLAT_HART_MAX,
              "platform.h 中 PLAT_BOOT_HART 不在内核可用的 hart 区间内");

// 把 hartid 换算成 cpuid: 平台差异 (QEMU: cpuid=hartid; VF2: cpuid=hartid-1)
// 由 PLAT_HART_MIN 描述。启动核由运行期决定, 不要用 boot_hart 换算。
int arch_hart_to_cpuid(uint64 hartid)
{
        return (int)(hartid - PLAT_HART_MIN);
}

uint64 arch_cpuid_to_hart(int cpuid)
{
        return (uint64)(cpuid + PLAT_HART_MIN);
}

int arch_cpu_id(void)
{
        return arch_hart_to_cpuid(arch_read_tp());
}

// 判断给定 hartid 是否由内核使用
bool arch_hart_is_valid(uint64 hartid)
{
        return hartid >= PLAT_HART_MIN && hartid <= PLAT_HART_MAX;
}

// ---- 冷启动核 (运行期事实, 见 arch.h 的说明) ----

// 由启动汇编写入 (entry.S): 冷启动核的 hartid + 1, 0 表示还没认领。
// 放在 .data 里, 不会被清 .bss 抹掉。
extern uint64 g_cold_boot_hartid;

// "冷启动核是谁"是否已被记下。记身份与清 .bss 在同一分支, 没记下即 .bss
// 未清, 是未定义状态, 必须停下来。
bool arch_cpu_boot_claim_ok(void)
{
        return g_cold_boot_hartid != 0;
}

uint64 arch_cpu_cold_boot_hart(void)
{
        return g_cold_boot_hartid - 1;
}

bool arch_cpu_is_boot_hart(void)
{
        return arch_cpu_boot_claim_ok() && arch_read_tp() == (g_cold_boot_hartid - 1);
}
