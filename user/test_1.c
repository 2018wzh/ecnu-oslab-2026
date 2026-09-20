/*
	测试一 基本能力
	1. 测试stdin stdout stderr fprintf的输入输出
	2. 测试exec的传参正确性
*/
#include "help.h"

char tmp1[] = "INPUT: ";
char tmp2[] = "OUTPUT: ";
char input[MAXLEN_STR + 1];

void test()
{
	stdout(tmp1, sizeof(tmp1) - 1);
	uint32 n = stdin(input, sizeof(input) - 1);
	input[n] = 0;
	stdout(tmp2, sizeof(tmp2) - 1);
	stdout(input, strlen(input));
	stderr(input, strlen(input));
}

void user_main(int argc, char *argv[])
{
	fprintf(STDOUT, "get %d argument:\n", argc);
	for (uint32 i = 0; i < (uint32)argc; i++)
		fprintf(STDOUT, "arg %d = %s\n", i, argv[i]);

	test();

	sys_exit(0);
}
