#ifndef OSLAB_STRING_H
#define OSLAB_STRING_H
#include <kernel/types.h>
/* 填充 len 字节为 value 的低 8 位，返回 dst。 */
void *memset(void *dst, int value, size_t len);
/* 复制 len 字节，源/目标不得重叠，返回 dst。 */
void *memcpy(void *dst, const void *src, size_t len);
/* 重叠安全复制 len 字节，返回 dst；零长度不访问内存。 */
void *memmove(void *dst, const void *src, size_t len);
/* 最多比较 len 字节，遇 NUL 停止；按无符号字节返回负/零/正。 */
int strncmp(const char *a, const char *b, size_t len);
#endif
