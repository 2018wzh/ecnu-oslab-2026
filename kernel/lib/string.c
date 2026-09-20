#include <kernel/string.h>
void *memset(void *dst, int value, size_t len)
{ uint8 *p = dst; for (size_t i = 0; i < len; i++) p[i] = (uint8)value; return dst; }
void *memcpy(void *dst, const void *src, size_t len)
{ uint8 *d = dst; const uint8 *s = src; for (size_t i = 0; i < len; i++) d[i] = s[i]; return dst; }
/* 使用地址整数判断方向，避免比较不相关对象的 C 指针。 */
void *memmove(void *dst, const void *src, size_t len)
{
    uint8 *d = dst;
    const uint8 *s = src;
    if ((uintptr_t)d < (uintptr_t)s) {
        for (size_t i = 0; i < len; ++i) d[i] = s[i];
    } else {
        while (len) { --len; d[len] = s[len]; }
    }
    return dst;
}
int strncmp(const char *a, const char *b, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
        if (x != y) return (int)x - (int)y;
        if (x == 0) return 0;
    }
    return 0;
}

/* 教师提供的本章辅助，不替代目录或路径任务。 */
int strlen(const char *str)
{ int i = 0; while (str[i] != '\0') ++i; return i; }
