// 启动其他 hart (通过 SBI HSM)
#include <kernel/types.h>
#include <kernel/print.h>
#include <platform.h>
#include <asm/sbi.h>
#include <asm/csr.h>

extern void _entry(void);

// 通过 SBI HSM 启动其他 hart。遍历平台声明的 hart 区间, 跳过运行期身份的自己;
// 启动核是谁由固件决定 (QEMU 冷启动核抽签), 不能拿 PLAT_BOOT_HART 当区间起点。
void smp_start_others(void)
{
        uint64 me = arch_read_tp();
        for (uint64 hartid = PLAT_HART_MIN; hartid <= PLAT_HART_MAX; hartid++) {
                if (hartid == me)
                        continue;
                // 只报失败。成功的从核会自己打印 "[cpu N] 启动完成"。
                int ret = sbi_hart_start(hartid, (uint64)_entry, hartid);
                if (ret != 0)
                        printf("[smp] 启动 hart %lu 失败 (err=%d)\n", hartid, ret);
        }
}
