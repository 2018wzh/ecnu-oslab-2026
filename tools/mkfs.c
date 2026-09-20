// 教师工具：默认排他创建稀疏镜像，仅显式 --force 截断重建。
#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <uapi/disk.h>
static void put32(unsigned char *p, uint32_t v)
{ for (unsigned i = 0; i < 4; ++i) p[i] = v >> (i * 8); }
int main(int argc, char **argv)
{
    int force = argc == 3 && strcmp(argv[1], "--force") == 0;
    if (argc != 2 && !force) { fprintf(stderr, "usage: mkfs [--force] IMAGE\n"); return 2; }
    const char *path = argv[force ? 2 : 1];
    FILE *f = fopen(path, force ? "wb" : "wbx"); if (!f) { perror(path); return 1; }
    const uint32_t fields[] = {FS_MAGIC, BLOCK_SIZE, TOTAL_BLOCKS, N_INODE,
        INODE_BITMAP_FIRST, INODE_BITMAP_BLOCKS, INODE_FIRST, INODE_BLOCKS, DATA_BITMAP_FIRST, DATA_BITMAP_BLOCKS, DATA_FIRST, N_DATA_BLOCK};
    unsigned char sb[BLOCK_SIZE] = {0};
    for (unsigned i = 0; i < 12; ++i) put32(sb + 4 * i, fields[i]);
    int failed = fwrite(sb, 1, sizeof(sb), f) != sizeof(sb);
    if (fflush(f) || ftruncate(fileno(f), (off_t)TOTAL_BLOCKS * BLOCK_SIZE)) failed = 1;
    if (fclose(f)) failed = 1;
    if (failed) { fprintf(stderr, "mkfs failed; incomplete image: %s\n", path); return 1; }
    return 0;
}
