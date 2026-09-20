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
/* 教师导入：保留根 inode 0 与 lab-8 种子；使用 10+2+1 索引格式。 */
static uint32_t next_data = 7;
static int at(FILE *f, uint64_t offset, const void *bytes, size_t len)
{ return fseeko(f, (off_t)offset, SEEK_SET) || fwrite(bytes, 1, len, f) != len; }
static uint32_t allocate(FILE *f)
{
    if (next_data >= N_DATA_BLOCK) return 0;
    uint32_t bit = next_data++, number = DATA_FIRST + bit;
    unsigned char byte = 0;
    if (fseeko(f, (off_t)DATA_BITMAP_FIRST * BLOCK_SIZE + bit / 8, SEEK_SET)) return 0;
    if (fread(&byte, 1, 1, f) != 1) return 0;
    byte |= 1U << (bit % 8);
    if (at(f, (uint64_t)DATA_BITMAP_FIRST * BLOCK_SIZE + bit / 8, &byte, 1)) return 0;
    return number;
}
static int index_set(FILE *f, uint32_t block, uint32_t slot, uint32_t value)
{ unsigned char bytes[4]; put32(bytes, value); return at(f, (uint64_t)block * BLOCK_SIZE + slot * 4, bytes, 4); }
static int import_file(FILE *f, unsigned inum, const char *path)
{
    const char *name = strrchr(path, '/'); name = name ? name + 1 : path;
    size_t namelen = strlen(name);
    /* 构建产物 test_N.elf 的目录名为 test_N。 */
    if (namelen > 4 && !strcmp(name + namelen - 4, ".elf")) namelen -= 4;
    if (!namelen || namelen >= MAXLEN_FILENAME || inum >= 63) return 1;
    FILE *src = fopen(path, "rb"); if (!src) return 1;
    unsigned char ip[64] = {0}, data[BLOCK_SIZE], entry[64] = {0};
    uint32_t indexes[13] = {0}, indirect = 0, previous_group = 0xffffffffU;
    uint64_t size = 0; unsigned logical = 0; size_t n;
    int failed = 0;
    while ((n = fread(data, 1, sizeof(data), src)) != 0) {
        if (size + n > 0xffffffffUL) { failed = 1; break; }
        memset(data + n, 0, sizeof(data) - n);
        uint32_t block = allocate(f); if (!block || write_block(f, block, data)) { failed = 1; break; }
        if (logical < 10) indexes[logical] = block;
        else if (logical < 10 + 2048) {
            unsigned slot = 10 + (logical - 10) / 1024;
            if (!indexes[slot]) indexes[slot] = allocate(f);
            if (!indexes[slot] || index_set(f, indexes[slot], (logical - 10) % 1024, block)) { failed = 1; break; }
        } else {
            unsigned rest = logical - 10 - 2048, group = rest / 1024;
            if (group >= 1024) { failed = 1; break; }
            if (!indexes[12]) indexes[12] = allocate(f);
            if (group != previous_group) {
                indirect = allocate(f); previous_group = group;
                if (!indexes[12] || !indirect || index_set(f, indexes[12], group, indirect)) { failed = 1; break; }
            }
            if (index_set(f, indirect, rest % 1024, block)) { failed = 1; break; }
        }
        size += n; ++logical;
    }
    if (ferror(src)) failed = 1;
    fclose(src); if (failed) return 1;
    put16(ip, INODE_TYPE_DATA); put16(ip + 2, INODE_MAJOR_DEFAULT); put16(ip + 4, INODE_MINOR_DEFAULT);
    put16(ip + 6, 1); put32(ip + 8, size);
    for (unsigned i = 0; i < 13; ++i) put32(ip + 12 + 4 * i, indexes[i]);
    if (at(f, (uint64_t)INODE_FIRST * BLOCK_SIZE + inum * 64, ip, 64)) return 1;
    unsigned char bit = 0;
    if (fseeko(f, (off_t)INODE_BITMAP_FIRST * BLOCK_SIZE + inum / 8, SEEK_SET) || fread(&bit, 1, 1, f) != 1) return 1;
    bit |= 1U << (inum % 8);
    if (at(f, (uint64_t)INODE_BITMAP_FIRST * BLOCK_SIZE + inum / 8, &bit, 1)) return 1;
    memcpy(entry, name, namelen); put32(entry + 60, inum);
    if (at(f, (uint64_t)DATA_FIRST * BLOCK_SIZE + (inum + 1) * 64, entry, 64)) return 1;
    unsigned char sz[4]; put32(sz, (inum + 2) * 64);
    return at(f, (uint64_t)INODE_FIRST * BLOCK_SIZE + 8, sz, 4);
}
int main(int argc, char **argv)
{
    int force = argc >= 3 && strcmp(argv[1], "--force") == 0;
    if (argc < 2 || (force && argc < 3)) { fprintf(stderr, "usage: mkfs [--force] IMAGE [ELF ...]\n"); return 2; }
    const char *path = argv[force ? 2 : 1];
    FILE *f = fopen(path, force ? "w+b" : "w+bx"); if (!f) { perror(path); return 1; }
    const uint32_t fields[] = {FS_MAGIC, BLOCK_SIZE, TOTAL_BLOCKS, N_INODE,
        INODE_BITMAP_FIRST, INODE_BITMAP_BLOCKS, INODE_FIRST, INODE_BLOCKS, DATA_BITMAP_FIRST, DATA_BITMAP_BLOCKS, DATA_FIRST, N_DATA_BLOCK};
    unsigned char sb[BLOCK_SIZE] = {0};
    for (unsigned i = 0; i < 12; ++i) put32(sb + 4 * i, fields[i]);
    int failed = fwrite(sb, 1, sizeof(sb), f) != sizeof(sb);
    if (fflush(f) || ftruncate(fileno(f), (off_t)TOTAL_BLOCKS * BLOCK_SIZE)) failed = 1;
    if (!failed && seed(f)) failed = 1;
    for (int i = force ? 3 : 2; i < argc && !failed; ++i)
        failed = import_file(f, 3 + i - (force ? 3 : 2), argv[i]);
    if (fclose(f)) failed = 1;
    if (failed) { fprintf(stderr, "mkfs failed; incomplete image: %s\n", path); return 1; }
    return 0;
}
