// 目录操作与路径解析。
// 目录在磁盘上就是连续 dirent 数组 (inum==0 表示空闲)。名字定长 14 字节且
// 不保证 '\0' 结尾: 定长可按下标直接定位, 代价是最长 14 字节。因此比较名字
// 必须用 strncmp(name, de->name, MAXNAME), 不能用 strcmp (越界读下一字段)。
#include <kernel/types.h>
#include <kernel/fs.h>
#include <kernel/print.h>
#include <kernel/string.h>
#include <kernel/sync.h>

/* 判断目录项是否为空 */
static int dirent_empty(dirent_t *de)
{
	return de->inum == 0;
}

/* --------------------------------------------------------------------------
 * 在目录中查找名字, 返回 inode 号 (0 表示没找到)
 * -------------------------------------------------------------------------- */

int dir_lookup(inode_t *dir, const char *name, uint32 *out_inum)
{
}
int dir_link(inode_t *dir, const char *name, uint32 inum)
{
}
inode_t *inode_by_path(const char *path)
{
	if (!path || path[0] != '/') {
		printf("[fs] 只支持绝对路径 (以 '/' 开头): '%s'\n", path);
		return NULL;
	}

	inode_t *ip = inode_get(ROOTINO);
	char name[MAXNAME + 1];
	const char *p = path;

	while (*p == '/')
		p++;

	while (*p) {
		/* 取出下一级名字 */
		int n = 0;
		while (*p && *p != '/' && n < MAXNAME) {
			name[n++] = *p++;
		}
		name[n] = '\0';
		while (*p == '/')
			p++;

		if (n == 0)
			break;

		/* 当前 inode 必须是目录 */
		if (ip->type != T_DIR) {
			printf("[fs] '%s' 不是目录\n", name);
			inode_put(ip);
			return NULL;
		}

		uint32 next_inum;
		if (!dir_lookup(ip, name, &next_inum)) {
			inode_put(ip);
			return NULL;   /* 不存在 */
		}

		inode_put(ip);
		ip = inode_get(next_inum);
	}

	return ip;
}

/* --------------------------------------------------------------------------
 * 解析路径的父目录, 并把最后一级名字回填到 name
 * 用于"创建文件"这类需要父目录 + 新名字的操作。
 * -------------------------------------------------------------------------- */
inode_t *inode_by_path_parent(const char *path, char *name)
{
	char buf[256];
	uint64 len = strlen(path);
	if (len >= sizeof(buf))
		return NULL;
	strcpy(buf, path);

	/* 去掉末尾的 '/' */
	while (len > 1 && buf[len - 1] == '/')
		buf[--len] = '\0';

	/* 找到最后一级名字 */
	char *slash = NULL;
	for (char *q = buf; *q; q++) {
		if (*q == '/')
			slash = q;
	}

	if (!slash || slash == buf) {
		/* 路径形如 "/name", 父目录就是根 */
		strcpy(name, slash ? slash + 1 : buf);
		return inode_get(ROOTINO);
	}

	*slash = '\0';
	strcpy(name, slash + 1);

	return inode_by_path(buf);
}

/* --------------------------------------------------------------------------
 * 创建一个新的 inode (文件或目录)
 * -------------------------------------------------------------------------- */
inode_t *inode_create(inode_t *dir, const char *name, uint16 type,
                      uint16 major, uint16 minor)
{
	/* 先找空闲的 inode 号 */
	uint32 inum = 0;
	for (uint32 i = 1; i < fs_superblock()->ninodes; i++) {
		inode_t *ip = inode_get(i);
		if (ip->type == 0) {
			/* 找到空闲的: 初始化它 */
			sleeplock_acquire(&ip->lock);
			ip->type = type;
			ip->major = major;
			ip->minor = minor;
			ip->nlink = 1;
			ip->size = 0;
			memset(ip->addrs, 0, sizeof(ip->addrs));
			inode_update(ip);
			sleeplock_release(&ip->lock);
			inum = i;
			inode_put(ip);
			break;
		}
		inode_put(ip);
	}

	if (inum == 0) {
		printf("[fs] inode_create: 没有空闲 inode\n");
		return NULL;
	}

	/* 在父目录里建立名字 -> inode 号的链接 */
	if (dir_link(dir, name, inum) < 0)
		return NULL;

	return inode_get(inum);
}
