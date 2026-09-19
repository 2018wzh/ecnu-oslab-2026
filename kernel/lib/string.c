// 最小字符串/内存操作 (generic, 无 libc)
#include <kernel/types.h>

void *memset(void *dst, int c, uint64 n)
{
        uint8 *d = dst;
        while (n--) *d++ = (uint8)c;
        return dst;
}

void *memmove(void *dst, const void *src, uint64 n)
{
        uint8 *d = dst;
        const uint8 *s = src;
        if (d < s) { while (n--) *d++ = *s++; }
        else { d += n; s += n; while (n--) *--d = *--s; }
        return dst;
}

void *memcpy(void *dst, const void *src, uint64 n)
{
        return memmove(dst, src, n);
}

int memcmp(const void *a, const void *b, uint64 n)
{
        const uint8 *x = a, *y = b;
        while (n--) { if (*x != *y) return *x - *y; x++; y++; }
        return 0;
}

uint64 strlen(const char *s)
{
        uint64 n = 0;
        while (s[n]) n++;
        return n;
}

int strcmp(const char *a, const char *b)
{
        while (*a && *a == *b) { a++; b++; }
        return (uint8)*a - (uint8)*b;
}

int strncmp(const char *a, const char *b, uint64 n)
{
        while (n && *a && *a == *b) { a++; b++; n--; }
        if (n == 0) return 0;
        return (uint8)*a - (uint8)*b;
}

char *strcpy(char *dst, const char *src)
{
        char *d = dst;
        while ((*d++ = *src++))
                ;
        return dst;
}
