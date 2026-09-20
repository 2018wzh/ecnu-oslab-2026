#include "sys.h"

void user_main(void)
{
	int pid = getpid();
	if (pid == 1) {
		print_str("\nproczero: hello ");
		print_str("world!\n");
	}
	while (1);
}
