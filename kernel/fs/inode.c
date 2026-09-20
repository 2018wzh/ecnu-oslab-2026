#include <kernel/inode.h>
#include <kernel/print.h>
#include <kernel/string.h>
#define MIN(a,b) ((a)<(b)?(a):(b))



/* 内存中的inode资源集合 */
static inode_t inode_cache[N_INODE_CACHE] __attribute__((unused));
static spinlock_t lk_inode_cache;

/* inode_cache初始化 */
void inode_init()
{
    panic("TODO(lab-8): inode_init");
}

/*--------------------关于inode->index的增删查操作-----------------*/

/*
	供free_data_blocks使用
	递归删除inode->index中的一个元素
	返回删除过程中是否遇到空的block_num (文件末尾)
*/
static bool __free_data_blocks(uint32 block_num, uint32 level)
{
    (void)block_num;
    (void)level;
    panic("TODO(lab-8): __free_data_blocks");
}

/*
	释放inode管理的blocks
*/
static __attribute__((unused)) void free_data_blocks(uint32 *inode_index)
{
	unsigned int i;
	bool meet_empty = false;

	/* step-1: 释放直接映射的block */
	for (i = 0; i < INODE_INDEX_1; i++)
	{
		meet_empty = __free_data_blocks(inode_index[i], 0);
		if (meet_empty) return;
	}

	/* step-2: 释放一级间接映射的block */
	for (; i < INODE_INDEX_2; i++)
	{
		meet_empty = __free_data_blocks(inode_index[i], 1);
		if (meet_empty) return;
	}

	/* step-3: 释放二级间接映射的block */
	for (; i < INODE_INDEX_3; i++)
	{
		meet_empty = __free_data_blocks(inode_index[i], 2);
		if (meet_empty) return;
	}

	panic("free_data_blocks: impossible!");
}

/*
	获取inode第logical_block_num个block的物理序号block_num
	调用者保证输入的logical_block_num只有两种情况:
	1. 属于已经分配的区域 (返回block_num)
	2. 将已经分配出去的区域往外扩展1个block (申请block并返回block_num)
	成功返回block_num, 失败返回-1
*/
static __attribute__((unused)) uint32 locate_or_add_block(uint32 *inode_index, uint32 logical_block_num)
{
    (void)inode_index;
    (void)logical_block_num;
    panic("TODO(lab-8): locate_or_add_block");
}

/*---------------------关于inode的管理: get dup lock unlock put----------------------*/

/*
	磁盘里的inode <-> 内存里的inode
    按 docs/disk-format.md 固定偏移 LE 编解码；不能直接复制宿主结构体。
	调用者需要持有ip->slk并设置合理的inode_num
*/
void inode_rw(inode_t *ip, bool write)
{
    (void)ip;
    (void)write;
    panic("TODO(lab-8): inode_rw");
}

/*
	尝试在inode_cache里寻找是否存在目标inode
	如果不存在则申请一个空闲的inode
	如果没有空闲位置直接panic
	核心逻辑: ref++
*/
inode_t *inode_get(uint32 inode_num)
{
    (void)inode_num;
    panic("TODO(lab-8): inode_get");
}

/*
	在磁盘里创建1个新的inode
	1. 查询和修改inode_bitmap
	2. 填充inode_region对应位置的inode
	注意: 返回的inode未上锁
*/
inode_t *inode_create(uint16 type, uint16 major, uint16 minor)
{
    (void)type;
    (void)major;
    (void)minor;
    panic("TODO(lab-8): inode_create");
}

/*
	ip->ref++ with lock proctect
*/
inode_t* inode_dup(inode_t* ip)
{
    (void)ip;
    panic("TODO(lab-8): inode_dup");
}

/*
	锁住inode
	如果inode->disk_info无效则更新一波
*/
void inode_lock(inode_t* ip)
{
    (void)ip;
    panic("TODO(lab-8): inode_lock");
}

/*
	解锁inode
*/
void inode_unlock(inode_t *ip)
{
    (void)ip;
    panic("TODO(lab-8): inode_unlock");
}

/*
	与inode_get相对应, 调用者释放inode资源
	如果达成某些条件, 可能触发彻底删除
*/
void inode_put(inode_t* ip)
{
    (void)ip;
    panic("TODO(lab-8): inode_put");
}

/*
	在磁盘里删除1个inode
	1. 修改inode_bitmap释放inode_region资源
	2. 修改block_bitmap释放block_region资源
	注意: 调用者需要持有ip->slk
*/
void inode_delete(inode_t *ip)
{
    (void)ip;
    panic("TODO(lab-8): inode_delete");
}

/*----------------------基于inode的数据读写操作--------------------*/

/*
	基于inode的数据读取
	inode管理的数据空间逻辑上是一个连续的数组data
	需要拷贝data[offset,offset+len)到dst(用户态地址/内核态地址)
	用户地址必须经页表检查与 copy_to_user；不得直接解引用用户地址。
	返回读取的数据量(字节)
*/
uint32 inode_read_data(inode_t *ip, uint32 offset, uint32 len, void *dst, bool is_user_dst)
{
    (void)ip;
    (void)offset;
    (void)len;
    (void)dst;
    (void)is_user_dst;
    panic("TODO(lab-8): inode_read_data");
}

/*
	基于inode的数据写入
	inode管理的数据空间逻辑上是一个连续的数组data
	需要拷贝src(用户态地址/内核态地址)到data[offset,offset+len)
	用户地址必须经页表检查与 copy_from_user；复制职责属于本章任务。
	返回写入的数据量(字节)
*/
uint32 inode_write_data(inode_t *ip, uint32 offset, uint32 len, void *src, bool is_user_src)
{
    (void)ip;
    (void)offset;
    (void)len;
    (void)src;
    (void)is_user_src;
    panic("TODO(lab-8): inode_write_data");
}

static char *inode_type_list[] = {"DATA", "DIR", "DEVICE"};

/* 输出inode信息(for debug) */
void inode_print(inode_t *ip, char* name)
{
	assert(sleeplock_holding(&ip->slk), "inode_print: slk");

	spinlock_acquire(&lk_inode_cache);

	printf("inode %s:\n", name);
	printf("ref = %d, inode_num = %d, valid_info = %d\n", ip->ref, ip->inode_num, ip->valid_info);
	printf("type = %s, major = %d, minor = %d, nlink = %d, size = %d\n", inode_type_list[ip->disk_info.type],
		ip->disk_info.major, ip->disk_info.minor, ip->disk_info.nlink, ip->disk_info.size);

	printf("index_list = [ ");
	for (int i = 0; i < INODE_INDEX_1; i++)
		printf("%d ", ip->disk_info.index[i]);
	printf("] [ ");
	for (int i = INODE_INDEX_1; i < INODE_INDEX_2; i++)
		printf("%d ", ip->disk_info.index[i]);
	printf("] [ ");
	for (int i = INODE_INDEX_2; i < INODE_INDEX_3; i++)
		printf("%d ", ip->disk_info.index[i]);
	printf("]\n\n");

	spinlock_release(&lk_inode_cache);
}
