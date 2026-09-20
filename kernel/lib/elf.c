// 教师 ELF64 LE 辅助：检查段大小、对齐和地址空间/磁盘偏移表示边界。
#include <kernel/elf.h>
#include <kernel/uvm.h>
#include <kernel/string.h>
#include <kernel/print.h>
#include <uapi/syscall.h>
static uint64 le(const uint8 *p, unsigned n)
{ uint64 v = 0; for (unsigned i = 0; i < n; ++i) v |= (uint64)p[i] << (8 * i); return v; }
int elf_read_header(inode_t *ip, elf_info_t *out) {
    uint8 h[64];
    if (inode_read_data(ip, 0, sizeof(h), h, false) != sizeof(h)) return -1;
    if (le(h, 4) != 0x464c457f || h[4] != 2 || h[5] != 1 || !arch_elf_machine(le(h + 18, 2))) return -1;
    out->entry = le(h + 24, 8); out->phoff = le(h + 32, 8); out->phnum = le(h + 56, 2);
    return 0;
}
/* 文件 [offset,offset+len) -> 已映射的用户区域 [va,va+len)。 */
void load_segment(inode_t *ip, pgtbl_t root, uint32 offset, uint64 va, uint32 len) {
    assert(va % PAGE_SIZE == 0, "load_segment: alignment");
    for (uint64 done = 0; done < len; done += PAGE_SIZE) {
        pte_t *pte = vm_getpte(root, va + done, false);
        assert(pte && (*pte & PTE_V), "load_segment: unmapped");
        uint32 n = len - done < PAGE_SIZE ? len - done : PAGE_SIZE;
        if (inode_read_data(ip, offset + done, n, (void *)PTE_TO_PA(*pte), false) != n)
            panic("load_segment: read fail");
    }
}
uint64 prepare_heap(pgtbl_t root, inode_t *ip, const elf_info_t *elf) {
    uint64 top = USER_ENTRY;
    for (unsigned i = 0; i < elf->phnum; ++i) {
        uint8 ph[56];
        uint64 off = elf->phoff + (uint64)i * sizeof(ph);
        if (off < elf->phoff || off > 0xffffffffUL - sizeof(ph)
            || inode_read_data(ip, off, sizeof(ph), ph, false) != sizeof(ph)) return ~0UL;
        if (le(ph, 4) != 1) continue;
        uint64 offset = le(ph + 8, 8), va = le(ph + 16, 8);
        uint64 filesz = le(ph + 32, 8), memsz = le(ph + 40, 8), end = va + memsz;
        if (memsz < filesz || end < va || va % PAGE_SIZE != 0) return ~0UL;
        // 继承最低页空洞/mmap 分区及 u32 inode 偏移，防止无符号长度下溢。
        if (va < USER_ENTRY || end < top || end > MMAP_BEGIN
            || offset > 0xffffffffUL || filesz > 0xffffffffUL - offset) return ~0UL;
        uint64 flags = PTE_R;
        if (le(ph + 4, 4) & 2) flags |= PTE_W;
        if (le(ph + 4, 4) & 1) flags |= PTE_X;
        top = uvm_heap_grow(root, top, end - top, flags);
        if (top != end) return ~0UL;
        load_segment(ip, root, offset, va, filesz);
    }
    return top;
}
/* 单页参数栈；参数长度包含 NUL，最大 128。每次下移均保持 16 字节对齐。 */
int prepare_stack(pgtbl_t root, const char *const argv[], size_t argc, uint64 *sp) {
    if (argc > MAX_ARGS) return -1;
    uint8 *page = pmem_alloc(false);
    memset(page, 0, PAGE_SIZE);
    uint64 bottom = USER_STACK_TOP - PAGE_SIZE, cursor = PAGE_SIZE, pointers[MAX_ARGS + 1] = {0};
    vm_mappages(root, bottom, (uint64)page, PAGE_SIZE, PTE_R | PTE_W | PTE_U);
    for (size_t i = 0; i < argc; ++i) {
        size_t len = 0;
        if (!argv[i]) return -1;
        while (len < ARG_BYTES && argv[i][len]) ++len;
        if (len == ARG_BYTES) return -1;
        size_t space = (len + 1 + 15) & ~15UL;
        if (space > cursor) return -1;
        cursor -= space; pointers[i] = bottom + cursor;
        memcpy(page + cursor, argv[i], len + 1);
    }
    size_t bytes = (argc + 1) * sizeof(uint64), space = (bytes + 15) & ~15UL;
    if (space > cursor) return -1;
    cursor -= space; memcpy(page + cursor, pointers, bytes);
    *sp = bottom + cursor; return 0;
}
