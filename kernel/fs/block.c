// 块设备注册与转发 (generic kernel), 放在 block.h 的接口背后。
#include <kernel/types.h>
#include <kernel/block.h>
#include <kernel/print.h>

static struct block_device *g_default;

void block_register(struct block_device *dev)
{
        g_default = dev;
        printf("[block] 注册块设备: %s (%lu 块 = %lu KB)\n",
               dev->name, dev->nblocks, dev->nblocks * BLOCK_SIZE / 1024);
}

struct block_device *block_default(void)
{
        return g_default;
}

int block_read(uint64 blockno, void *buf, uint32 nblocks)
{
}

int block_write(uint64 blockno, const void *buf, uint32 nblocks)
{
}
