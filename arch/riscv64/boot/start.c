// 从固件交接进入内核 C 世界 (启动适配层核心)。
// 职责: 1) 把固件寄存器参数变成内核可用形式; 2) 建立 C 执行所需的最小机器态;
// 3) 调用 generic 的 main()。本版不使用 BootInfo: 平台信息是编译期宏,
// 无需运行时传递; 若要支持 DTB 探测再引入运行时结构。
#include <kernel/types.h>
#include <kernel/print.h>
#include <kernel/arch.h>
#include <platform.h>
#include <asm/csr.h>
#include <asm/sbi.h>

// 由 generic kernel 提供
extern void main(void);

// start(): 内核 C 语言的第一个函数。
// 参数按 RISC-V 调用约定: a0 = hartid, a1 = dtb。
void start(uint64 hartid, uint64 dtb_addr)
{
	// hartid: 启动核编号, 会写进 tp 供 arch_cpu_id() 读;
	// dtb_addr: 设备树地址, 本课程用编译期 platform.h 描述硬件, 不用它,
	//           但保留形参以遵守 RISC-V S-mode 启动 ABI。
	(void)dtb_addr;

	// ---- 1. 确认分页是关闭的 ----
	// 固件应已清零 satp, 若不为 0 说明固件行为与预期不符。
	arch_write_satp(0);

	// ---- 2. 把 hartid 写进 tp ----
	// entry.S 已做过, 这里再确认: 可能有别的启动路径 (SBI HSM) 直接跳进来。
	arch_write_tp(hartid);

	// ---- 3. 使能 S-mode 的三类中断源 ----
	// sie 控制哪些中断源被允许 (SEIE/STIE/SSIE), sstatus.SIE 是全局总开关,
	// 两者必须同时为 1 中断才送达。总开关由 main() 等在 trap 向量装好后再开。
	SET_CSR(sie, SIE_SEIE | SIE_STIE | SIE_SSIE);

	// ---- 4. 关闭所有中断总开关, 等内核自己按需打开 ----
	// trap 向量未设好前来了中断会跳到未初始化的 stvec, 必须提前关掉。
	arch_irq_disable();

	// ---- 5. 设置一个安全的 trap 向量 ----
	// 正式 trap 系统初始化前, 先把 stvec 指向死循环, 异常时"停下"而非野指针。
	extern void trap_early_park(void);
	arch_write_stvec((uint64)trap_early_park);

	// ---- 6. 打印启动横幅 (走 SBI, 不依赖 UART 驱动) ----
	// 只有冷启动核打印: 从核输出会与启动核交错成乱码。抽签已在 entry.S 完成。
	if (arch_cpu_is_boot_hart()) {
		arch_early_puts("\n");
		arch_early_puts("[oslab] kernel booting via ");
		arch_early_puts(BOOT_NAME);
		arch_early_puts(" on ");
		arch_early_puts(PLAT_NAME);
		arch_early_puts("\n");
	}

	// ---- 7. 地址约定由编译期与链接期保证, 运行期不再检查 ----
	// 链接脚本 ASSERT 保证实际布局, 编译期断言保证依赖固件的协议不把内核
	// 加载进固件区, 都比运行期检查更早更准确。
#ifdef CONFIG_BOOT_HAS_FIRMWARE
	STATIC_ASSERT(EXPECTED_LOAD_ADDR >= PLAT_KERNEL_BASE,
	              "内核加载地址落在固件区里 (本启动协议依赖固件)");
#endif

	// ---- 8. 进入 generic kernel ----
	main();

	// main() 不应该返回
	arch_early_puts("[oslab] FATAL: main() 意外返回\n");
	for (;;)
		asm volatile("wfi");
}
