/* 内存管理接口 (generic kernel): 定义物理页、地址空间、映射、权限等通用概念;
 * 页表项编码等架构细节在 arch 层, generic VM 算法不拼页表位。 */
#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include <kernel/types.h>
#include <kernel/sync.h>

// PGSIZE / PGSHIFT 是架构事实, 由 <kernel/arch_mm.h> 提供, 不写死在此 (换页大小架构时无需改 generic)。
#include <kernel/arch_mm.h>

// 由链接脚本提供。
extern char ALLOC_BEGIN[];
extern char KERNEL_END[];

// 内存区域: 一段能被分配器管理的物理内存。
typedef struct mem_region {
	uint64 begin;
	uint64 end;
	spinlock_t lk;
	uint32 free_pages;
	struct page_node *free_list;
} mem_region_t;

typedef struct page_node {
	struct page_node *next;
} page_node_t;

// 物理内存分配 (kernel/mm/pmem.c)。
// 初始化物理内存分配器: 分内核专用区与用户可用区。
void pmem_init(void);

// 分配一个物理页返回其物理地址 (失败返回 0), zero 为真时清零。
// 返回的是物理地址, 当前内核是恒等映射所以可直接当指针用。
uint64 pmem_alloc(bool zero);

// 分配一个用户页 (与内核页分开管理)。
uint64 pmem_alloc_user(bool zero);

// 释放一个物理页, 自动判断属于内核区还是用户区。
void pmem_free(uint64 pa);

// 显式释放用户页。
void pmem_free_user(uint64 pa);

// 统计空闲页数。
void pmem_stat(uint32 *kernel_free, uint32 *user_free);

// 页表操作 (arch 层提供实现)。
// 页表根类型由架构决定 (RISC-V 下是物理页号)。
typedef uint64 pgtbl_t;

// 创建一个空页表返回其物理地址, 失败返回 0。
pgtbl_t kvm_create(void);

// 在页表中建立 va->pa 映射, 大小 size, 权限 perm; 返回 0 成功, 负数失败。
int kvm_map(pgtbl_t pgtbl, uint64 va, uint64 pa, uint64 size, int perm);

// 解除映射。
void kvm_unmap(pgtbl_t pgtbl, uint64 va, uint64 size);

// 查询 va 对应的物理地址, 未映射返回 0。
uint64 kvm_translate(pgtbl_t pgtbl, uint64 va);

// 复制内核页表 (用于创建用户进程页表, 共享内核部分)。
pgtbl_t kvm_copy_kernel(pgtbl_t kernel_pgtbl);

// 内核页表全局句柄。
pgtbl_t kvm_global(void);

// 初始化内核地址空间。
void kvm_init(void);

// 每个 hart 的地址空间初始化 (激活内核页表)。
void kvm_init_hart(void);

// 用户地址空间 (kernel/mm/uvmmap.c)。
// 在用户地址空间分配并映射内存 (立即分配物理页); 返回起始地址, 失败返回 0。
uint64 uvm_alloc(pgtbl_t pgtbl, uint64 va, uint64 size, int perm);

// 内核->用户拷贝。用户指针不可信, 必须查页表校验, 不要直接用 memmove。
// 返回 0 成功, -1 表示用户地址非法。
int uvm_copyout(pgtbl_t pgtbl, uint64 dst_va, uint64 src_kva, uint64 len);

// 用户->内核拷贝, src_va 是用户地址。
int uvm_copyin(pgtbl_t pgtbl, uint64 dst_kva, uint64 src_va, uint64 len);

// 从用户地址读取以 '\0' 结尾的字符串, 最多 max 字节; 0 成功, -1 地址非法或过长。
int uvm_copyin_str(pgtbl_t pgtbl, char *dst, uint64 src_va, uint64 max);

// 释放整个用户地址空间。
void uvm_free(pgtbl_t pgtbl, uint64 sz);

// 权限位 (架构无关抽象, arch 层翻译成实际 PTE 位); 定义在 <kernel/perm.h>
// 因为 arch_perm_to_pte() 也要用, arch 不应反向包含 generic 的内存管理头。
#include <kernel/perm.h>

#endif /* __KERNEL_MM_H__ */
