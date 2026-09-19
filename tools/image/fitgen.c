// FIT (Flattened Image Tree) 镜像生成器 (宿主工具)。
// 自己写而非用 mkimage: 教学环境不保证有 u-boot-tools、少外部依赖少"我机器上能
// 跑"问题、学生能理解 U-Boot 怎么识别镜像而非敲魔法命令、可复现不依赖版本差异。
// FIT 就是带 FDT 头的容器: [FDT 头][结构块(节点/属性)][字符串块(属性名表)][数据(内核
// 二进制)]。U-Boot bootm 解析这棵树校验 arch, 把 data 搬到 load 地址再跳到 entry。
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FDT_MAGIC   0xd00dfeed
#define FDT_BEGIN_NODE 0x1
#define FDT_END_NODE   0x2
#define FDT_PROP       0x3
#define FDT_END        0x9

static uint8_t *sb;      /* structure block */
static size_t   sb_len, sb_cap;
static uint8_t *st;      /* strings block */
static size_t   st_len, st_cap;

static void sb_reserve(size_t n)
{
	if (sb_len + n > sb_cap) {
		sb_cap = (sb_cap ? sb_cap * 2 : 4096) + n;
		sb = realloc(sb, sb_cap);
		if (!sb) { perror("realloc"); exit(1); }
	}
}

static void st_reserve(size_t n)
{
	if (st_len + n > st_cap) {
		st_cap = (st_cap ? st_cap * 2 : 1024) + n;
		st = realloc(st, st_cap);
		if (!st) { perror("realloc"); exit(1); }
	}
}

static void put32be(uint8_t *p, uint32_t v)
{
	p[0] = (v >> 24) & 0xff; p[1] = (v >> 16) & 0xff;
	p[2] = (v >> 8) & 0xff;  p[3] = v & 0xff;
}

static void sb_align(void)
{
	while (sb_len % 4) { sb_reserve(1); sb[sb_len++] = 0; }
}

static void sb_begin_node(const char *name)
{
	sb_align();
	sb_reserve(4);
	put32be(sb + sb_len, FDT_BEGIN_NODE); sb_len += 4;
	size_t n = strlen(name) + 1;
	sb_reserve(n);
	memcpy(sb + sb_len, name, n); sb_len += n;
}

static void sb_end_node(void)
{
	sb_align();
	sb_reserve(4);
	put32be(sb + sb_len, FDT_END_NODE); sb_len += 4;
}

/* 把属性名放进字符串块, 返回其偏移 */
static uint32_t st_add(const char *s)
{
	size_t n = strlen(s) + 1;
	st_reserve(n);
	uint32_t off = (uint32_t)st_len;
	memcpy(st + st_len, s, n);
	st_len += n;
	return off;
}

static void sb_prop_raw(const char *name, const void *val, size_t len)
{
	sb_align();
	sb_reserve(12);
	put32be(sb + sb_len, FDT_PROP); sb_len += 4;
	put32be(sb + sb_len, (uint32_t)len); sb_len += 4;
	put32be(sb + sb_len, st_add(name)); sb_len += 4;
	sb_reserve(len);
	memcpy(sb + sb_len, val, len); sb_len += len;
}

static void sb_prop_str(const char *name, const char *val)
{
	sb_prop_raw(name, val, strlen(val) + 1);
}

static void sb_prop_u32(const char *name, uint32_t v)
{
	uint8_t b[4];
	put32be(b, v);
	sb_prop_raw(name, b, 4);
}

static uint32_t crc32_tab[256];
static void crc32_init(void)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t c = i;
		for (int k = 0; k < 8; k++)
			c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
		crc32_tab[i] = c;
	}
}

static uint32_t crc32(const uint8_t *p, size_t n)
{
	uint32_t c = 0xFFFFFFFFu;
	for (size_t i = 0; i < n; i++)
		c = crc32_tab[(c ^ p[i]) & 0xff] ^ (c >> 8);
	return c ^ 0xFFFFFFFFu;
}

int main(int argc, char *argv[])
{
	if (argc != 6) {
		fprintf(stderr,
		        "用法: %s <内核bin> <输出itb> <加载地址hex> <入口地址hex> <描述>\n",
		        argv[0]);
		return 1;
	}

	const char *kern_path = argv[1];
	const char *out_path  = argv[2];
	uint32_t load_addr  = (uint32_t)strtoul(argv[3], NULL, 0);
	uint32_t entry_addr = (uint32_t)strtoul(argv[4], NULL, 0);
	const char *desc = argv[5];

	/* ---- 读取内核二进制 ---- */
	FILE *f = fopen(kern_path, "rb");
	if (!f) { fprintf(stderr, "无法打开 %s\n", kern_path); return 1; }
	fseek(f, 0, SEEK_END);
	long klen = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *kdata = malloc(klen);
	if (fread(kdata, 1, klen, f) != (size_t)klen) {
		fprintf(stderr, "读取内核失败\n"); return 1;
	}
	fclose(f);

	crc32_init();
	uint32_t kcrc = crc32(kdata, klen);

	/* ---- 构建结构块与字符串块 ---- */
	sb_begin_node("");

	sb_prop_str("description", desc);
	sb_prop_u32("#address-cells", 1);

	/* images/kernel 节点 */
	sb_begin_node("images");
	sb_begin_node("kernel");
	sb_prop_str("description", "OSLab kernel");
	/* data 属性用 data-offset/data-size 表示 (由 mkimage 的 -E 模式使用;
	 * 这里我们用外部数据模式: 属性值为空, 真正的数据放在镜像尾部,
	 * 并用 data-position/data-offset 指明位置)。
	 * 为简化并保证 U-Boot 兼容, 采用"属性长度 0 + data-size"的写法:
	 * U-Boot 会按 data-offset 从本 FDT 的数据区读取。 */
	sb_prop_u32("data-size", (uint32_t)klen);
	sb_prop_u32("data-offset", 0);        /* 稍后回填 */
	sb_prop_u32("data-position", 0);      /* 稍后回填 */
	sb_prop_str("type", "kernel");
	sb_prop_str("arch", "riscv");
	sb_prop_str("os", "linux");
	sb_prop_str("compression", "none");
	sb_prop_u32("load", load_addr);
	sb_prop_u32("entry", entry_addr);
	/* 真正的 FIT 里这里还可以有 hash-1 子节点声明 CRC 校验。
	 * 本实现不声明校验和 —— U-Boot 允许无校验的镜像,
	 * 教学场景下"能加载并启动"比"校验完整"更重要。
	 * 学生可以自己加上 hash 节点作为扩展练习。 */
	sb_end_node();  /* 结束 kernel */
	sb_end_node();  /* 结束 images */

	sb_begin_node("configurations");
	sb_prop_str("default", "conf-1");
	sb_begin_node("conf-1");
	sb_prop_str("description", desc);
	sb_prop_str("kernel", "kernel");
	sb_end_node();
	sb_end_node();

	sb_end_node();  /* 根节点 */

	sb_align();
	sb_reserve(4);
	put32be(sb + sb_len, FDT_END); sb_len += 4;
	uint32_t off_rsvmap  = 40;
	uint32_t off_struct  = off_rsvmap + 16;
	uint32_t off_strings = off_struct + (uint32_t)sb_len;
	uint32_t strings_padded = (uint32_t)((st_len + 3) & ~3u);
	uint32_t off_data    = off_strings + strings_padded;
	uint32_t total       = off_data + (uint32_t)((klen + 3) & ~3u);

	uint8_t *img = calloc(1, total);
	if (!img) { perror("calloc"); return 1; }

	/* FDT 头 */
	put32be(img + 0,  FDT_MAGIC);
	put32be(img + 4,  total);
	put32be(img + 8,  off_struct);
	put32be(img + 12, off_strings);
	put32be(img + 16, off_rsvmap);    /* off_mem_rsvmap */
	put32be(img + 20, 17);            /* version */
	put32be(img + 24, 16);            /* last_comp_version */
	put32be(img + 28, 0);             /* boot_cpuid_phys */
	put32be(img + 32, (uint32_t)st_len); /* size_dt_strings */
	put32be(img + 36, (uint32_t)sb_len); /* size_dt_struct */

	/* 内存保留块: 两个 8 字节全零表示"没有保留区域" (calloc 已保证为 0) */
	memcpy(img + off_struct, sb, sb_len);
	memcpy(img + off_strings, st, st_len);
	memcpy(img + off_data, kdata, klen);

	/* 回填 data-offset / data-position (相对整个 FIT 镜像) */
	/* 由于上面写的是简化结构, 这里用一次简单的扫描替换:
	 * 查找 data-offset/data-position 属性的值位置并写入正确值。 */
	{
		/* 结构块中属性顺序: data-size, data-offset, data-position
		 * 每个属性头 12 字节, 数据紧随其后。这里通过两次线性查找定位。 */
		for (uint32_t i = 0; i + 12 <= sb_len; i += 4) {
			uint32_t tag, nameoff;
			tag = ((uint32_t)sb[i] << 24) | (sb[i+1] << 16) | (sb[i+2] << 8) | sb[i+3];
			if (tag != FDT_PROP) continue;
			nameoff = ((uint32_t)sb[i+8] << 24) | (sb[i+9] << 16) | (sb[i+10] << 8) | sb[i+11];
			if (nameoff >= st_len) continue;
			const char *nm = (const char *)(st + nameoff);
			uint32_t voff = off_struct + i + 12;
			if (!strcmp(nm, "data-offset"))
				put32be(img + voff, off_data);
			else if (!strcmp(nm, "data-position"))
				put32be(img + voff, off_data);
		}
	}

	/* 重新计算总长度并更新头 */
	put32be(img + 4, total);

	FILE *o = fopen(out_path, "wb");
	if (!o) { fprintf(stderr, "无法创建 %s\n", out_path); return 1; }
	fwrite(img, 1, total, o);
	fclose(o);

	printf("fitgen: %s -> %s\n", kern_path, out_path);
	printf("fitgen:   内核大小 %ld 字节, 加载地址 0x%08x, 入口 0x%08x\n",
	       klen, load_addr, entry_addr);
	printf("fitgen:   镜像总大小 %u 字节, 内核数据偏移 %u\n", total, off_data);
	printf("fitgen:   内核 CRC32 = 0x%08x\n", kcrc);

	free(img); free(kdata); free(sb); free(st);
	return 0;
}
