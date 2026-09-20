#ifndef OSLAB_SD_H
#define OSLAB_SD_H
#include <kernel/types.h>
/* 教师后端：启动独占初始化；请求由 block 条件锁串行化。 */
int sd_init(void);
int sd_submit(uint64 data_pa, uint32 block, bool write);
int sd_complete(int *result);
#endif
