# LAB-8: 文件系统 之 数据组织与层次结构

**前言**

在lab-7中我们实现了block-level的磁盘管理能力

在此基础上, 本次实验将进一步考虑以下两个问题:

1. 如果数据块的大小大于1个block, 如何组织和管理? (inode)

2. 如何用人类更好理解的层次结构来组织和索引海量数据块? (dentry)

## 代码组织结构

```
ECNU-OSLAB-2026
├── pictures       README使用的图片目录 (CHANGE)
├── README.md      实验指导书 (CHANGE)
├── include
│   ├── kernel/inode.h (NEW, inode与目录接口)
│   └── uapi/disk.h (CHANGE, 磁盘结构常量)
├── kernel/fs
│   ├── inode.c (TODO, 核心工作)
│   ├── dentry.c (TODO, 核心工作)
│   ├── fs.c (TODO, 增加inode初始化逻辑和测试用例)
│   └── lab8_examples.c (NEW, 测试用例)
└── tools/mkfs.c (CHANGE, 带根目录的磁盘映像)
```

**标记说明**

**NEW**: 新增源文件, 直接拷贝即可, 无需修改

**CHANGE**: 旧的源文件发生了更新, 直接拷贝即可, 无需修改

**TODO**: 你需要实现新功能 / 你需要完善旧功能

## mkfs: 带根目录的初始磁盘映像

lab-7的初始磁盘映像虽然有inode_bitmap和inode_region, 但是并不存在真正的inode

在本次实验中, 我们首先添加了根目录 (root_inode)

随后, 在根目录之下增加了四个目录项:

- "." 和 "..": 特殊目录项, 用于描述相对路径, 本次实验不展开描述

- "ABCD.txt" 和 "abcd.txt": 循环写入字母表(大写版本和小写版本)的普通文件

请你阅读`include/uapi/disk.h`和`tools/mkfs.c`来理解初始化过程的具体行为, 这将帮助你理解**inode**和**dentry**的概念

## inode: 文件数据组织

从磁盘角度来看, 文件就是由N个block构成的逻辑单元

inode负责记录这样的逻辑结构, 主要依靠**index**字段和**size**字段

**index字段的设计分成三个部分:**

- 0 ~ INODE_INDEX_1 - 1: 直接映射(index[i] = data_block), 控制前面40KB空间 (小型文件)

- INODE_INDEX_1 ~ INODE_INDEX_2 - 1: 一级间接映射(index[i] = index_block), 控制中间8MB空间 (中型文件)

- INODE_INDEX_2 ~ INODE_INDEX_3 - 1: 二级间接映射(index[i] = index_index_block), 控制后面4GB空间 (大型文件)

**这样设计的好处是:**

- 小型文件的访问非常迅速 (直接访问inode就能直接获得data_block序号)

- 能支持很大的文件 (以1个index_index_block和1024个index_block的额外访问开销为代价)

- 中型文件的访问代价可控 (以2个index_block的额外访问开销为代价)

**和之前的内核页表设计进行对比:**

- index是各个文件的私有映射逻辑, 内核页表是系统全局的映射逻辑, 因此内存页面不存在前面热后面冷的性质

- 因此, 页表是确定的三级间接映射, 文件映射则是直接映射、一级间接映射、二级间接映射相结合(按需启用)

**关于size字段(单位是字节)的说明:**

- 对于**INODE_TYPE_DATA**类型的inode, size除了代表有效的数据量, 还代表[0, size)的逻辑空间已经使用 (没有空洞)

- 对于**INODE_TYPE_DIR**类型的inode, 我们假设它只使用1个block(index[0]记录), size只代表有效数据量 (接受空洞)

**当你完全理解上述逻辑后, 我们开始进入具体的函数 (in inode.c)**

首先阅读帮助函数`free_data_blocks`, 实现它调用的`__free_data_blocks`和`locate_or_add_block`

`free_data_blocks`本质是通过`bitmap_free_block`释放inode管理的所有block资源

考虑到树形组织结构, 我们使用递归方法来实现这一点 (调用`__free_data_blocks`)

注意: 起到索引作用的index_block和index_index_block也要释放

`locate_or_add_block`负责将逻辑块号转译为物理块号 (例:inode的第一个逻辑块0对应物理块1120)

转译过程中可能遇到目标逻辑块号恰好超过边界 (例:分配了N个逻辑块, 目标逻辑块号为N)

这种时候需要新分配一个物理块, 使得目标逻辑块号合法

比较复杂的情况: 新增data_block可能带来连锁反应 (新增index_index_block和新增index_block)

这一点和页表的生长过程是类似的, 需要你细心和谨慎地处理

之后实现数据流读写逻辑`inode_read_data`和`inode_write_data`

它们的共同点在于: 以1个buffer为中间载体, 实现数据在磁盘块和通用内存空间之间的流转

需要注意的一点: 写入逻辑可能涉及inode的修改 (包括index和size), 文件大小不能超过INODE_MAX_SIZE (size为32位无符号数, 最多表示4294967295字节)

## inode: 生命周期管理

我们使用**inode_cache**来管理内存中的inode资源, 通过**lk_inode_cache**来保护它

这种组织结构在`mmap`、`proc`、`buffer`等地方多次遇到, 这里也是类似的 (只是没有选择使用双向循环链表做加速)

请你完成`inode_init`来初始化资源, 并放入`fs_init`的合适位置

下一个需要思考的问题是: **inode in disk** 与 **inode in memory**的区别与互相更新

```c
/* 磁盘上的索引节点(64 Byte) */
typedef struct inode_disk {
	uint16 type;                          // 文件类型
	uint16 major;                         // 主设备号
	uint16 minor;                         // 次设备号
	uint16 nlink;                         // 链接数
	unsigned int size;                   // 文件数据长度(字节)
	unsigned int index[INODE_INDEX_3];   // 数据存储位置(10+2+1)
} inode_disk_t;

/* 内存里的索引节点 */
typedef struct inode {
	inode_disk_t disk_info;           // 持久化信息 (slk保护)
	bool valid_info;                  // disk_info的有效性 (slk保护)
	uint32 inode_num;                 // inode序号 (活跃引用期间不变)
	uint32 ref;                       // 引用数 (lk_inode_cache保护)
	sleeplock_t slk;                  // 睡眠锁
} inode_t;
```

相比**inode_disk_t**, **inode_t** 增加了四个字段:

- valid_info: inode_cache miss后返回的空闲inode, 它的disk_info是无效的, 需要特殊标记

- ref: inode_cache中的inode可以被多个使用者关注, 需要记录一下引用数

- inode_num: 进行**inode_disk_t**和**inode_t**的互相更新时, 需要知道磁盘中的inode位置

- slk: 保证资源的按需共享, 由于磁盘读写非常耗时, 所以采用睡眠锁而非自旋锁

之后请你实现`inode_rw`完成磁盘inode_region中的inode和内存inode_cache中的inode的互相更新。磁盘字段按固定偏移用小端格式读写, 不直接复制内存结构体。

- 初始化时: 通常是 磁盘->内存 的更新流 (读入一个新的inode)

- 修改时: 通常是 内存->磁盘 的更新流 (inode中的字段做了修改, 需要写回)

下面实现典型的inode生命周期控制函数 (按照从生到死的过程)：

- `inode_create`: 认为某个inode原本不存在, 先在内存里创建副本, 之后写入inode_region

- `inode_get`: 认为某个inode存在于内存inode_cache或者磁盘inode_region, 获取使用权

- `inode_dup`: 复制inode使用权 (例如执行fork操作时)

- `inode_lock`: 获取睡眠锁以保证独占inode

- `inode_unlock`: 释放睡眠锁以支持共享inode

- `inode_put`: 释放inode的使用权(ref--), 可能触发inode的磁盘删除操作

- `inode_delete`: 删除磁盘里的某个inode, 并释放它管理的data_block资源

最后, 我们提供了`inode_print`函数来输出某个inode的具体信息

## dentry: 从数字索引到字符串索引

**有了inode的文件系统世界是什么样的呢?**

- 我们可以先用**inode_num**搜索**inode_region**, 找到某个inode (定位元数据)

- 再通过inode里的索引信息, 按照逻辑顺序找到它管理的若干**block** (定位数据)

对于计算机来说, 这套逻辑已经足够高效了, 它建立了从数字到离散数据流的映射关系

对人来说, 还存在一个致命的问题:**inode_num**太抽象了, 人类很难记住它的含义, 人更好理解的是**name**

我们可以把 **name=hello.c** 理解为一个"hello world"程序, 但是不能把 **inode_num=15** 理解成任何东西

**因此, 我们决定增加一层映射逻辑: name -> inode_num**

```c
/* 目录项(64 Byte) */
typedef struct dentry {
    char name[MAXLEN_FILENAME];       // 文件名
    unsigned int inode_num;           // 索引节点序号
} dentry_t;
```

更具体的设计方案:

- 我们将第一个inode设为**根节点**, 记住它的inode_num为**ROOT_INODE(0)**

- 根节点包括 `BLOCK_SIZE/sizeof(dentry_t)` 个**dentry槽位**

- 其他inode都通过注册1个dentry链接到根节点上, 形成**1-N**的二层树形结构

- 查找逻辑从: inode_num->数据 变成了 ROOT_INODE->name->inode_num->数据

用户只需要记住根节点的inode序号和目标inode的名称即可, 不需要知道目标inode的序号

请你实现dentry的槽位管理逻辑:

- `dentry_search`: 在目录中查找某个dentry, 找不到时返回INVALID_INODE_NUM, 不能用合法的根目录编号0表示失败

- `dentry_create`: 创建1个新的dentry

- `dentry_delete`: 删除1个旧的dentry

我们还提供了一个`dentry_print`用于打印某个目录下的所有有效dentry (dentry->name[0]!='\0'说明有效)

## path: 从扁平化到层次化

**1-N**的扁平化树形结构在inode数量较小时是好用的

随着inode数量的增长: 一方面, 根节点的槽位不够用; 另一方面, 不方便人做分类查找和管理

因此, 我们要将扁平化的树拓展为层次化的树, 支持`1-N-M-K...`的多层结构

管理这样的结构需要多个目录类型的inode (仅靠根节点是不够的)

描述树上的一个节点需要使用"文件路径(path)", 本次实验我们只考虑绝对路径

例如: /AAA/BBB/CCC/file.txt、/AABB/CC、////AAA///CC//BB/file.txt等

以/AAA/BBB/CCC.txt为例, 解释**路径解析**过程:

- step-0: 通过ROOT_INODE(0)获得root_inode

- step-1: 查询root_inode的dentry槽位, 发现"AAA"对应的inode_num为A1

- step-2: 通过A1获得AAA_inode

- step-3: 查询AAA_inode的dentry槽位, 发现"BBB"对应的inode_num为B1

- step-4: 通过B1获得BBB_inode

- step-5: 查询BBB_inode的dentry槽位, 发现"CCC.txt"对应的inode_num为C1

- step-6: 通过C1获得CCC_inode, 进行后续的读写行为

我们提供了`get_element`来逐步处理**path**和提取**name**, 请你先理解它的工作逻辑

你需要利用`get_element`、`inode_get`、`dentry_search`等函数来实现`__path_to_inode`

这个函数完成了从**path**到**目标inode**的翻译过程, 是`path_to_inode`和`path_to_parent_inode`的底层

## 测试用例

考虑到测试的便捷性, 请直接在`fs_init`的最后位置添加一组测试逻辑, 替换原有的`lab8_examples()`调用。测试2还需在**kernel/fs/fs.c**中包含**kernel/mem.h**。

**测试1: inode的访问 + 创建 + 删除**

测试现象示意:

![pic](./pictures/01.png)

```c
    /* fs_init in fs.c */

	printf("============= test begin =============\n\n");

	inode_t *rooti, *ip_1, *ip_2;
	
	rooti = inode_get(ROOT_INODE);
	inode_lock(rooti);
	inode_print(rooti, "root");
	inode_unlock(rooti);

	/* 第一次查看bitmap */
	bitmap_print(false);

	ip_1 = inode_create(INODE_TYPE_DIR, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	ip_2 = inode_create(INODE_TYPE_DATA, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	inode_lock(ip_1);
	inode_lock(ip_2);
	inode_dup(ip_2);

	inode_print(ip_1, "dir");
	inode_print(ip_2, "data");
	
	/* 第二次查看bitmap */
	bitmap_print(false);

	ip_1->disk_info.nlink = 0;
	ip_2->disk_info.nlink = 0;
	inode_unlock(ip_1);
	inode_unlock(ip_2);
	inode_put(ip_1);
	inode_put(ip_2);

	/* 第三次查看bitmap */
	bitmap_print(false);

	inode_put(ip_2);
	
	/* 第四次查看bitmap */
	bitmap_print(false);

	printf("============= test end =============\n\n");

	while(1);
```

**测试2: 写入和读取inode管理的数据**

测试现象示意:

![pic](./pictures/02.png)

```c
    /* fs_init in fs.c */

	printf("============= test begin =============\n\n");

	inode_t *ip_1, *ip_2;
	uint32 len, cut_len;

	/* 小批量读写测试 */

	int small_src[10], small_dst[10];
	for (int i = 0; i < 10; i++)
		small_src[i] = i;
	
	ip_1 = inode_create(INODE_TYPE_DATA, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	inode_lock(ip_1);
	inode_print(ip_1, "small_data");

	printf("writing data...\n\n");
	cut_len = 10 * sizeof(int);
	for (uint32 offset = 0; offset < 400 * cut_len; offset += cut_len) {
		len = inode_write_data(ip_1, offset, cut_len, small_src, false);
		assert(len == cut_len, "write fail 1!");
	}
	inode_print(ip_1, "small_data");

	len = inode_read_data(ip_1, 120 * cut_len + 4, cut_len, small_dst, false);
	assert(len == cut_len, "read fail 1!");
	printf("read data:");
	for (int i = 0; i < 10; i++)
		printf(" %d", small_dst[i]);
	printf("\n\n");

	ip_1->disk_info.nlink = 0;
	inode_unlock(ip_1);
	inode_put(ip_1);

	/* 大批量读写测试 */

	char *big_src, big_dst[9];
	big_dst[8] = 0;

	/* 申请五个物理页面, 检查它们是否连续 */
	big_src = pmem_alloc(true);
	assert(pmem_alloc(true) == big_src + PAGE_SIZE, "contiguous fail!");
	assert(pmem_alloc(true) == big_src + PAGE_SIZE * 2, "contiguous fail!");
	assert(pmem_alloc(true) == big_src + PAGE_SIZE * 3, "contiguous fail!");
	assert(pmem_alloc(true) == big_src + PAGE_SIZE * 4, "contiguous fail!");

	for (uint32 i = 0; i < 5 * (PAGE_SIZE / 8); i++)
		for (uint32 j = 0; j < 8; j++)
			big_src[i * 8 + j] = 'A' + j;

	ip_2 = inode_create(INODE_TYPE_DATA, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	inode_lock(ip_2);
	inode_print(ip_2, "big_data");

	printf("writing data...\n\n");
	cut_len = PAGE_SIZE * 4 + 1110;
	for (uint32 offset = 0; offset < cut_len * 10000; offset += cut_len)
	{
		len = inode_write_data(ip_2, offset, cut_len, big_src, false);
		assert(len == cut_len, "write fail 2!");
	}
	inode_print(ip_2, "big_data");

	len = inode_read_data(ip_2, cut_len * 10000 - 8, 8, big_dst, false);
	assert(len == 8, "read fail 2");
	printf("read data: %s\n", big_dst);


	ip_2->disk_info.nlink = 0;
	inode_unlock(ip_2);
	inode_put(ip_2);

	pmem_free((uint64)big_src, true);
	pmem_free((uint64)big_src + PAGE_SIZE, true);
	pmem_free((uint64)big_src + PAGE_SIZE * 2, true);
	pmem_free((uint64)big_src + PAGE_SIZE * 3, true);
	pmem_free((uint64)big_src + PAGE_SIZE * 4, true);


	printf("============= test end =============\n");

	while(1);
```

**测试3: 目录项的增加、删除、查找操作**

测试现象示意:

![pic](./pictures/03.png)

```c
    /* fs_init in fs.c */
	printf("============= test begin =============\n\n");

	inode_t *rooti, *ip_1, *ip_2, *ip_3;
	uint32 inode_num_1, inode_num_2, inode_num_3;
	uint32 len, cutlen, offset;
	char tmp[10];

	tmp[9] = 0;
	cutlen = 9;
	rooti = inode_get(ROOT_INODE);

	/* 搜索预置的dentry */

	inode_lock(rooti);
	inode_num_1 = dentry_search(rooti, "ABCD.txt");
	inode_num_2 = dentry_search(rooti, "abcd.txt");
	inode_num_3 = dentry_search(rooti, ".");
	if (inode_num_1 == INVALID_INODE_NUM || 
		inode_num_2 == INVALID_INODE_NUM || 
		inode_num_3 == INVALID_INODE_NUM) {
		panic("invalid inode num!");
	}
	dentry_print(rooti);
	inode_unlock(rooti);

	ip_1 = inode_get(inode_num_1);
	inode_lock(ip_1);
	ip_2 = inode_get(inode_num_2);
	inode_lock(ip_2);
	ip_3 = inode_get(inode_num_3);
	inode_lock(ip_3);

	inode_print(ip_1, "ABCD.txt");
	inode_print(ip_2, "abcd.txt");
	inode_print(ip_3, "root");

	len = inode_read_data(ip_1, 0, cutlen, tmp, false);
	assert(len == cutlen, "read fail 1!");
	printf("\nread data: %s\n", tmp);

	len = inode_read_data(ip_2, 0, cutlen, tmp, false);
	assert(len == cutlen, "read fail 2!");
	printf("read data: %s\n\n", tmp);

	inode_unlock(ip_1);
	inode_unlock(ip_2);
	inode_unlock(ip_3);
	inode_put(ip_1);
	inode_put(ip_2);
	inode_put(ip_3);

	/* 创建和删除dentry */
	inode_lock(rooti);

	ip_1 = inode_create(INODE_TYPE_DIR, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);	
	offset = dentry_create(rooti, ip_1->inode_num, "new_dir");
	inode_num_1 = dentry_search(rooti, "new_dir");
	printf("new dentry offset = %d\n", offset);
	printf("new dentry inode_num = %d\n\n", inode_num_1);
	
	dentry_print(rooti);

	inode_num_2 = dentry_delete(rooti, "new_dir");
	assert(inode_num_1 == inode_num_2, "inode num is not equal!");

	dentry_print(rooti);

	inode_unlock(rooti);
	inode_put(rooti);

	printf("============= test end =============\n");

	while(1);
```

**测试4: 文件路径的解析**

测试现象示意:

![pic](./pictures/04.png)

```c
    /* fs_init in fs.c */

    printf("============= test begin =============\n\n");

	inode_t *rooti, *ip_1, *ip_2, *ip_3, *ip_4, *ip_5;
	
	/* 准备测试环境 */

	rooti = inode_get(ROOT_INODE);
	ip_1 = inode_create(INODE_TYPE_DIR, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	ip_2 = inode_create(INODE_TYPE_DIR, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	ip_3 = inode_create(INODE_TYPE_DATA, INODE_MAJOR_DEFAULT, INODE_MINOR_DEFAULT);
	
	inode_lock(rooti);
	inode_lock(ip_1);
	inode_lock(ip_2);
	inode_lock(ip_3);

	if (dentry_create(rooti, ip_1->inode_num, "AABBC") == (uint32)-1)
		panic("dentry_create fail 1!");
	if (dentry_create(ip_1, ip_2->inode_num, "aaabb") == (uint32)-1)
		panic("dentry_create fail 2!");
	if (dentry_create(ip_2, ip_3->inode_num, "file.txt") == (uint32)-1)
		panic("dentry_create fail 3!");

	char tmp1[] = "This is file context!";
	char tmp2[32];
	inode_write_data(ip_3, 0, sizeof(tmp1), tmp1, false);

	inode_rw(rooti, true);
	inode_rw(ip_1, true);
	inode_rw(ip_2, true);

	inode_unlock(rooti);
	inode_unlock(ip_1);
	inode_unlock(ip_2);
	inode_unlock(ip_3);
	inode_put(rooti);
	inode_put(ip_1);
	inode_put(ip_2);
	inode_put(ip_3);

	char *path = "///AABBC///aaabb/file.txt";
	char name[MAXLEN_FILENAME];

	ip_4 = path_to_inode(path);
	if (ip_4 == NULL)
		panic("invalid ip_4");

	ip_5 = path_to_parent_inode(path, name);
	if (ip_5 == NULL)
		panic("invalid ip_5");
	
	printf("get a name = %s\n\n", name);

	inode_lock(ip_4);
	inode_lock(ip_5);

	inode_print(ip_4, "file.txt");
	inode_print(ip_5, "aaabb");

	inode_read_data(ip_4, 0, 32, tmp2, false);
	printf("read data: %s\n\n", tmp2);

	inode_unlock(ip_4);
	inode_unlock(ip_5);
	inode_put(ip_4);
	inode_put(ip_5);

	printf("============= test end =============\n");
```

**尾声**

本次实验我们实现了两个文件系统的基石: inode 和 dentry

它们分别定义了文件系统的数据组织逻辑和层次化逻辑, 希望能帮助你理解文件系统的构建逻辑

在lab-9中, 我们将先基于这两块基石构建**普通文件和目录文件**的管理逻辑

之后, 秉持**一切皆文件**的Linux设计哲学, 我们还将介绍一类特殊的文件——**设备文件**

最后, 我们还将讨论进程模块与文件系统的关系, 并补全进程模块的最后一块拼图——**proc_exec**

**lab-9 既是文件系统的最终章、也是内核全系统的粘合剂、还是迄今为止最困难的终极考核!**

## 进阶目标

### 日志文件系统

创建一个文件时, 位图、inode和目录都可能需要更新。如果写到一半突然断电, 这些信息就可能互相矛盾。日志文件系统会先记录一组相关修改, 让重启后的内核能够判断这组操作是否完成, 并据此恢复。

请你先以创建文件为例, 确定哪些修改需要一起完成, 再尝试设计日志记录、提交标志和恢复顺序。在不同写入位置模拟断电, 重启后检查目录、链接数和位图是否一致, 验证后再扩大日志覆盖的操作范围。

### VFS 挂载

不同文件系统组织磁盘数据的方式可能不同, 但都需要提供文件查找、目录遍历等操作。VFS用一组共同接口连接这些实现, 挂载则让一个文件系统出现在另一个文件系统的目录下。

请你尝试从inode、目录和路径操作中提取共同接口, 先在一个目录上挂载第二个只读实例。观察跨挂载点的路径解析, 测试根目录、`.`、`..`以及仍有引用时的卸载, 注意区分不同文件系统中相同的inode编号。完成lab-9的文件接口后, 可以继续验证文件访问。

### 统一 page cache

本次实验按磁盘块缓存数据, 文件读写需要先把文件内偏移换成块号。如果改为按文件和页号缓存, 文件读写与文件内存映射就可以使用同一份内存副本。修改过但尚未写回磁盘的页面称为脏页, 回收前需要处理这些修改。

请你尝试设计这样的缓存, 可以先确定如何标识文件页、记录脏页和安排写回, 再考虑它与现有块缓存的关系。结合后续文件与映射实验, 观察不同方式访问同一文件时能否读到一致的数据, 并检查写回失败或页面仍在使用时该如何处理回收。
