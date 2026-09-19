// 把嵌入的 initcode 变成内核可链接的符号。
// initcode.h 只有数组【定义】; 多个 .c include 会重复定义, 只声明不定义又链接
// 不到。用单独一个编译单元 include 产生唯一定义, 其他文件 extern 声明引用
// (见 include/kernel/elf.h)。这是"生成代码进构建"的常见模式。
#include <initcode.h>
