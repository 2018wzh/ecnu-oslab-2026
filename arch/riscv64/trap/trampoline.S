// 用户态<->内核态切换的公共代码页, 必须严格占一页 (链接脚本 ASSERT 保证)。
// lab-4 实现完整逻辑。
.section trampsec
.global trampoline
trampoline:
        .space 4096
