#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <dune/dune.h>
#include <vmpl/log.h>

/* Handlers. */
static void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf)
{
	ptent_t *pte;
#if CONFIG_VMPL_DEBUG
	printf("syscall: caught page fault, addr=%ld\n", addr);
#endif
	dune_vm_lookup(pgroot, (void *) addr, 0, &pte);
	*pte |= PTE_P | PTE_W | PTE_U | PTE_A | PTE_D;
}

static void syscall_handler(struct dune_tf *tf)
{
	log_debug("syscall: caught syscall, num=%ld\n", tf->rax);
	dune_passthrough_syscall(tf);
}

/* Utility functions. */
static int dune_call_user(void *func)
{
	int ret;
	unsigned long sp;
	struct dune_tf *tf = malloc(sizeof(struct dune_tf));
	if (!tf)
		return -ENOMEM;

	asm ("movq %%rsp, %0" : "=r" (sp));
	tf->rip = (unsigned long) func;
	tf->rsp = sp - 0x10008;
	tf->rflags = 0x0;

	// 从这里开始rsp未对齐
	ret = dune_jump_to_user(tf);

	return ret;
}

static void user_mmap_test()
{
	int rc;
	void *addr, *tmp_addr;
	pte_t *ptep;
	pte_t *new_root;

	// Test mmap
	log_info("Test mmap");
	addr = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
	assert(addr != MAP_FAILED);
	log_success("Test mmap passed");

	// Test access to the mapped page
	log_info("Test access to the mapped page");
	*(uint64_t *)(addr) = 0xdeadbeef;
	assert(*(uint64_t *)addr == 0xdeadbeef);
	log_success("Test access to the mapped page passed");

	// Test mmap at a specific address that is already mapped with MAP_FIXED_NOREPLACE.
	tmp_addr = addr;
	log_info("Test mmap at a specific address that is already mapped with MAP_FIXED_NOREPLACE");
	addr = mmap(tmp_addr, PGSIZE, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	log_info("addr: %p, tmp_addr: %p, errno: %d", addr, tmp_addr, errno);
	assert(addr == MAP_FAILED);
	assert(errno == EEXIST);
	log_success("Test mmap at a specific address that is already mapped with MAP_FIXED_NOREPLACE passed");

	// Test mmap at a specific address that is already mapped with MAP_FIXED.
	log_info("Test mmap at a specific address that is already mapped with MAP_FIXED");
	addr = mmap(tmp_addr, PGSIZE, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	log_info("addr: %p, tmp_addr: %p, errno: %d", addr, tmp_addr, errno);
	assert(addr != MAP_FAILED);
	assert(addr == tmp_addr);
	log_success("Test mmap at a specific address that is already mapped with MAP_FIXED passed");

	// Test mprotect
	log_info("Test mprotect");
	rc = mprotect(addr, PGSIZE, PROT_READ);
	assert(rc == 0);
	log_success("Test mprotect passed");

	// Test pkey_alloc
	log_info("Test pkey_alloc");
	int pkey = pkey_alloc(0, PKEY_DISABLE_WRITE);
	assert(pkey >= 0);
	log_success("Test pkey_alloc passed");

	// Test pkey_free
	log_info("Test pkey_free");
	rc = pkey_free(pkey);
	assert(rc == 0);
	log_success("Test pkey_free passed");

	// Test pkey_mprotect
	log_info("Test pkey_mprotect");
	pkey = pkey_alloc(0, PKEY_DISABLE_WRITE);
	rc = pkey_mprotect(addr, PGSIZE, PROT_READ, pkey);
	assert(rc == 0);
	log_success("Test pkey_mprotect passed");

	// Test mremap
	log_info("Test mremap");
	addr = mremap(addr, PGSIZE, PGSIZE * 2, MREMAP_MAYMOVE, NULL);
	assert(addr != MAP_FAILED);
	log_success("Test mremap passed");

	// Test munmap
	log_info("Test munmap");
	rc = munmap(addr, PGSIZE * 2);
	assert(rc == 0);
	log_success("Test munmap passed");
}

/* User code with system calls. */
void user_main()
{
	int ret = syscall(SYS_gettid);
	assert(ret > 0);
	printf("syscall: gettid=%d\n", ret);
	user_mmap_test();
	dune_ret_from_user(ret);
}

int main(int argc, char *argv[])
{
	int ret;

	printf("syscall: not running dune yet\n");

	ret = dune_init_and_enter();
	if (ret) {
		printf("failed to initialize dune\n");
		return ret;
	}

	printf("syscall: now printing from dune mode\n");

	dune_register_pgflt_handler(pgflt_handler);
	dune_register_syscall_handler(syscall_handler);

	printf("syscall: about to call user code\n");
	return dune_call_user(&user_main);
}

