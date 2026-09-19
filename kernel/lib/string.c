#include <kernel/string.h>
void *memset(void *dst, int value, size_t len)
{ uint8 *p = dst; for (size_t i = 0; i < len; i++) p[i] = (uint8)value; return dst; }
void *memcpy(void *dst, const void *src, size_t len)
{ uint8 *d = dst; const uint8 *s = src; for (size_t i = 0; i < len; i++) d[i] = s[i]; return dst; }
