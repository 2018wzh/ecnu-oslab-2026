// 裸机直启: 没有从核可启动。
// 启动从核需要固件的 HSM 扩展; 无固件时这一步为空。
// 想真正用上多核: 自己写 M-mode 从核入口 + CLINT MSIP 核间中断 (见 docs/porting.md)。
#include <kernel/types.h>

void smp_start_others(void)
{
        // 单核启动: 无事可做
}
