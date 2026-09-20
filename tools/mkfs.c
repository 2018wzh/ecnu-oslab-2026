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
static void put16(unsigned char *p, uint16_t v)
{ p[0] = v; p[1] = v >> 8; }
static int write_block(FILE *f, uint32_t block, const unsigned char *data)
{ return fseeko(f, (off_t)block * BLOCK_SIZE, SEEK_SET) || fwrite(data, 1, BLOCK_SIZE, f) != BLOCK_SIZE; }
/* 教师提供的初始目录和普通文件；所有字段显式 LE 编码。 */
static int seed(FILE *f)
{
    unsigned char block[BLOCK_SIZE] = {0};
    block[0] = 7; /* inode 0、1、2 */
    if (write_block(f, INODE_BITMAP_FIRST, block)) return 1;
    block[0] = 127; /* 根目录 1 块 + 大写文件 2 块 + 小写文件 4 块 */
    if (write_block(f, DATA_BITMAP_FIRST, block)) return 1;
    memset(block, 0, sizeof(block));
    const uint32_t sizes[] = {256, 5200, 13000};
    const uint32_t first[] = {DATA_FIRST, DATA_FIRST + 1, DATA_FIRST + 3};
    for (unsigned i = 0; i < 3; ++i) {
        unsigned char *ip = block + i * DISK_INODE_SIZE;
        put16(ip, i == 0 ? INODE_TYPE_DIR : INODE_TYPE_DATA);
        put16(ip + 2, INODE_MAJOR_DEFAULT); put16(ip + 4, INODE_MINOR_DEFAULT);
        put16(ip + 6, 1); put32(ip + 8, sizes[i]);
        for (unsigned j = 0; j < (sizes[i] + BLOCK_SIZE - 1) / BLOCK_SIZE; ++j)
            put32(ip + 12 + j * 4, first[i] + j);
    }
    if (write_block(f, INODE_FIRST, block)) return 1;
    memset(block, 0, sizeof(block));
    for (unsigned i = 0; i < BLOCK_SIZE / 64; ++i) put32(block + i * 64 + 60, INVALID_INODE_NUM);
    const char *names[] = {".", "..", "ABCD.txt", "abcd.txt"};
    const uint32_t numbers[] = {ROOT_INODE, ROOT_INODE, 1, 2};
    for (unsigned i = 0; i < 4; ++i) {
        memcpy(block + i * 64, names[i], strlen(names[i]));
        put32(block + i * 64 + 60, numbers[i]);
    }
    if (write_block(f, DATA_FIRST, block)) return 1;
    for (unsigned file = 1; file < 3; ++file) {
        for (unsigned offset = 0; offset < sizes[file]; offset += BLOCK_SIZE) {
            memset(block, 0, sizeof(block));
            for (unsigned j = 0; j < BLOCK_SIZE && offset + j < sizes[file]; ++j)
                block[j] = (file == 1 ? 'A' : 'a') + (offset + j) % 26;
            if (write_block(f, first[file] + offset / BLOCK_SIZE, block)) return 1;
        }
    }
    return 0;
}
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
    if (!failed && seed(f)) failed = 1;
    if (fclose(f)) failed = 1;
    if (failed) { fprintf(stderr, "mkfs failed; incomplete image: %s\n", path); return 1; }
    return 0;
}
