#include "sys.h"
static unsigned length(const char *s) { unsigned n = 0; while (s[n]) ++n; return n; }
void user_main(void) {
    // 手动选择 path/argv；默认 test_1，保留交互输入等待。
    char path[] = "./test_1";
    char *argv[] = {"test_1", "111", "222", "333", 0};
    int pid = (int)sys_fork();
    if (pid < 0) sys_write(2, sizeof("initcode: fork fail!\n") - 1, "initcode: fork fail!\n");
    else if (pid == 0) {
        sys_write(1, 4, "run "); sys_write(1, length(path), path);
        for (unsigned i = 0; argv[i]; ++i) { sys_write(1, 1, " "); sys_write(1, length(argv[i]), argv[i]); }
        char start[] = "\n======== test start ========\n\n";
        sys_write(1, sizeof(start) - 1, start);
        sys_exec(path, argv);
        char fail[] = "initcode: exec fail!\n";
        sys_write(2, sizeof(fail) - 1, fail); sys_exit(1);
    } else {
        uint32 status = 0;
        int waited = (int)sys_wait(&status);
        char ok[] = "\n======== test success ========\n";
        char fail[] = "\n======== test fail ========\n";
        if (waited >= 0 && status == 0) sys_write(1, sizeof(ok) - 1, ok);
        else sys_write(1, sizeof(fail) - 1, fail);
    }
    for (;;) {}
}
