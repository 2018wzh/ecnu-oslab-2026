#include <kernel/proc.h>
#include <kernel/file.h>
#include <kernel/print.h>
#include <kernel/console.h>
#include <kernel/arch.h>
typedef long (*device_read_fn)(void *, size_t);
typedef long (*device_write_fn)(const void *, size_t);
typedef struct { char name[MAXLEN_FILENAME]; device_read_fn read; device_write_fn write; } device_t;
static device_t device_table[7]; /* major 0 无效，1～6 对应本章设备。 */
/* 注册设备（教师辅助）。仅在初始化阶段、设备表尚无并发读者时调用。
 * 验证后一次填入名称与操作；不替学生选择设备、权限或创建 /dev 节点。
 * 学生完成初始化任务前允许辅助函数暂未使用。
 */
static int __attribute__((unused)) device_register(uint16 major, const char *name,
                                                  device_read_fn read, device_write_fn write)
{
    if (!major || major >= 7 || !name || (!read && !write)) return -1;
    size_t len = 0;
    while (len < MAXLEN_FILENAME && name[len]) ++len;
    if (!len || len == MAXLEN_FILENAME) return -1;
    device_t entry = { .read = read, .write = write };
    for (size_t i = 0; i < len; ++i) entry.name[i] = name[i];
    device_table[major] = entry;
    return 0;
}
// 教师提供的设备行为；学生实现设备表、权限检查及分派。
long device_stdin(void *dst, size_t len) { return console_read(dst, len); }
long device_stdout(const void *src, size_t len)
{ const uint8 *p = src; for (size_t i = 0; i < len; ++i) console_putc(p[i]); return len; }
long device_stderr(const void *src, size_t len)
{ device_stdout("ERROR: ", 7); return device_stdout(src, len); }
long device_zero(void *dst, size_t len)
{ uint8 *p = dst; for (size_t i = 0; i < len; ++i) p[i] = 0; return len; }
long device_null_read(void *dst, size_t len) { (void)dst; (void)len; return 0; }
long device_null_write(const void *src, size_t len) { (void)src; return len; }
static bool question_is(const uint8 *src, size_t len, const char *question)
{
    size_t i = 0;
    while (question[i]) { if (i == len || src[i] != (uint8)question[i]) return false; ++i; }
    return i == len;
}
long device_gpt(const void *src, size_t len)
{
    const uint8 *text = src;
    size_t n = len;
    while (n && (text[n - 1] == '\n' || text[n - 1] == '\r')) --n;
    if (question_is(text, n, "Hello")) {
        printf("Hi, I am gpt0!\n");
    } else if (question_is(text, n, "Guess who I am")) {
        char name[17] = {0};
        push_off();
        proc_t *p = myproc();
        int pid = p ? p->pid : 0;
        if (p) for (unsigned i = 0; i < sizeof(p->name); ++i) name[i] = p->name[i];
        pop_off();
        printf("Your procid is %d and name is %s.\n", pid, name);
    } else if (question_is(text, n, "How many free memory left")) {
        uint64 kernel_pages = pmem_stat(true), user_pages = pmem_stat(false);
        printf("We have %d free pages in kernel space, %d free pages in user space!\n",
               (int)kernel_pages, (int)user_pages);
    } else if (question_is(text, n, "Good job")) {
        printf("Thanks for your kind words!\n");
    } else {
        printf("Sorry, I can not understand it.\n");
    }
    return len;
}
// major: 1 stdin、2 stdout、3 stderr、4 zero、5 null、6 gpt0；minor=INODE_MINOR_DEFAULT。
// TODO(lab-9): 用 device_register 初始化设备表；建立 /dev 及六个设备 inode，重复启动不得重复创建。
int device_init(void) { panic("TODO(lab-9): device_init"); }
// TODO(lab-9): stdin/zero 只读，stdout/stderr/gpt0 只写，null 可读写。
bool device_open_check(uint16 major, uint32 mode)
{ (void)major; (void)mode; panic("TODO(lab-9): device_open_check"); }
// TODO(lab-9): stdin 行缓冲读取，zero 填零，null EOF，其余返回错误。
long device_read(uint16 major, void *dst, size_t len)
{ (void)major; (void)dst; (void)len; panic("TODO(lab-9): device_read"); }
// TODO(lab-9): stdout 输出，stderr 加 ERROR 前缀，null 丢弃，gpt0 固定问答；返回输入字节数。
long device_write(uint16 major, const void *src, size_t len)
{ (void)major; (void)src; (void)len; panic("TODO(lab-9): device_write"); }
