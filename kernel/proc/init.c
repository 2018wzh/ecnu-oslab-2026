// 创建第一个用户进程。
// 完整启动链条: OpenSBI/U-Boot -> _entry(建栈) -> start(最小机器态) ->
// main(初始化) -> idle 就绪 + trap/时钟就位 -> proc_make_first() 创建第一个
// 用户进程 -> user_enter() 进用户态 -> initcode main() 用户态开始执行。
#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/arch_mm.h>

/* initcode 二进制数组 (由 mk/build.mk 用 xxd 生成, 见 initcode_blob.c)。
 * xxd -i 把数组名设成输入文件名, 所以生成的就是 initcode / initcode_len。 */
extern unsigned char initcode[];
extern unsigned int  initcode_len;

void proc_make_first(void)
{
}
