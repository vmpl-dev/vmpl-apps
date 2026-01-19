#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vmpl/sys.h>
#include <dune/dune.h>

static void recover(void)
{
	printf("hello: recovered from divide by zero\n");
	exit(0);
}

static void divide_by_zero_handler(struct dune_tf *tf)
{
	printf("hello: caught divide by zero!\n");
	tf->rip = (uintptr_t)&recover;
}

int main(int argc, char *argv[])
{
	volatile int ret;

	printf("hello: not running dune yet\n");

	ret = dune_init_and_enter();
	if (ret) {
		printf("failed to initialize dune\n");
		return ret;
	}

	printf("hello: now printing from dune mode\n");

	dune_register_intr_handler(T_DE, divide_by_zero_handler);

	ret = 1 / ret; /* divide by zero */

	printf("hello: we won't reach this call\n");

	return 0;
}