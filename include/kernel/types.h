/* 基础类型, 架构无关: 无论哪种 ISA, uint64 都应是 64 位, 故属于 generic kernel。 */
#ifndef __KERNEL_TYPES_H__
#define __KERNEL_TYPES_H__

typedef char                int8;
typedef short               int16;
typedef int                 int32;
typedef long long           int64;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

// 寄存器宽度类型: "一个通用寄存器能装下的整数", 64 位架构上等于 uint64,
// 用此名表达"这个值是地址/长度, 宽度跟着架构走"。
typedef unsigned long       reg_t;

typedef unsigned long       size_t;
typedef long                ssize_t;

// C23 起 bool/true/false 是语言关键字, 只能按 __STDC_VERSION__ 判断再定义;
// 同一份代码在 GCC 12 与 GCC 16 上默认行为不同, 故内核须显式指定语言标准。
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
  /* C23: bool/true/false 由语言提供, 无需定义 */
#else
  typedef enum { false = 0, true = 1 } bool;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

// 编译期断言: 条件不成立则编译失败, 比运行时 assert 更早暴露错误。
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

// 确保一个值落在区间内。
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 对齐运算。
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

// 把指针转成整数或反过来 (访问 MMIO 时常用)。
#define PTR2INT(x) ((uint64)(x))
#define INT2PTR(x) ((void *)(x))

#endif /* __KERNEL_TYPES_H__ */
