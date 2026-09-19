// 内存/字符串操作 (generic kernel, 无 libc)。
#ifndef __KERNEL_STRING_H__
#define __KERNEL_STRING_H__

#include <kernel/types.h>

void  *memset(void *dst, int c, uint64 n);
void  *memmove(void *dst, const void *src, uint64 n);
void  *memcpy(void *dst, const void *src, uint64 n);
int    memcmp(const void *a, const void *b, uint64 n);
uint64 strlen(const char *s);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, uint64 n);
char  *strcpy(char *dst, const char *src);

#endif
