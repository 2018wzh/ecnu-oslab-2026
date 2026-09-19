/* 页权限的架构无关表示: 四种权限是概念, 每个架构都有只是编码不同,
 * 翻译成实际 PTE 位只在 arch 层的 arch_perm_to_pte() 一处发生。 */
#ifndef __KERNEL_PERM_H__
#define __KERNEL_PERM_H__

#define PERM_R 0x1   /* 可读 */
#define PERM_W 0x2   /* 可写 */
#define PERM_X 0x4   /* 可执行 */
#define PERM_U 0x8   /* 用户态可访问 */

#endif /* __KERNEL_PERM_H__ */
