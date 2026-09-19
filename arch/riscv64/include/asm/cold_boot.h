// 冷启动核抽签用的常量 (架构相关)。
// 启动汇编 entry.S 与 C 代码 cpu.c 都要用这两个常量, 放一起保证单一来源,
// 避免抄错导致"抽签永远不生效"而 QEMU 上看起来正常。
#ifndef __ASM_COLD_BOOT_H__
#define __ASM_COLD_BOOT_H__

// 抽签锁初值 (哨兵)。非零魔数而非 0: 0 会进 .bss, 在 objcopy 裸二进制路径上
// 不被加载成垃圾, 抽签结果不可预测; 放 .data 两条路径都读得到。
// 最高位须为 0 或比较时零扩展: amoswap.w 只换 32 位却符号扩展成 64 位, 哨兵
// 0xB007B007 被扩成 0xffffffff_b007b007 与常数永不相等, 每核都当自己来晚。
#define LOTTERY_FREE 0xB007B007

// 输家等赢家写下身份的自旋上限 (见 entry.S 的 .Lwait_loop)。正常一两圈就够,
// 上限只为"哨兵是垃圾谁都没赢"的病态情况留出路。
#define CLAIM_SPIN   1048576

#endif /* __ASM_COLD_BOOT_H__ */
