// 早期 trap 兜底点。
// lab-1 就需要 (start.c 把 stvec 指向它作为安全落脚点), 完整 trap 入口是 lab-4
// 的内容, 分开才能让 lab-1 只编译自己需要的部分。
.section .text
.global trap_early_park
trap_early_park:
        // 在此发生异常时"死循环"而不是跳进未初始化地址。
        wfi
        j       trap_early_park
