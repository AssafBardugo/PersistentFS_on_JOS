// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	void* pgaddr = (void*)(PGNUM(addr) << PGSHIFT);
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if(!(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW))
		panic("pgfault: page fault can't handling here\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	envid_t curr_envid = 0;

	if((r = sys_page_alloc(curr_envid, (void*)PFTEMP, PTE_P | PTE_U | PTE_W)) < 0)
		panic("pgfault: sys_page_alloc return %e\n", r);

	memcpy((void*)PFTEMP, pgaddr, PGSIZE);

	if((r = sys_page_map(curr_envid, (void*)PFTEMP, curr_envid, pgaddr, PTE_P | PTE_U | PTE_W)) < 0)
		panic("pgfault: sys_page_map return %e\n", r);

	if((r = sys_page_unmap(curr_envid, (void*)PFTEMP)) < 0)
		panic("pgfault: sys_page_unmap return %e\n", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function? 
// ************ANSWER: )
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	envid_t parent_envid;
	void* pgaddr;

	parent_envid = 0;
	pgaddr = (void*)(pn << PGSHIFT); 

	switch(uvpt[pn] & (PTE_W | PTE_COW)){

		case PTE_W:
		case PTE_COW:

			if((r = sys_page_map(parent_envid, pgaddr, envid, pgaddr, PTE_P | PTE_U | PTE_COW)) < 0){

				cprintf("duppage: sys_page_map return %e\n for the child\n", r);
				return r;
			}
		
			if((r = sys_page_map(parent_envid, pgaddr, parent_envid, pgaddr, PTE_P | PTE_U | PTE_COW)) < 0){

				cprintf("duppage: sys_page_map return %e\n for the parent\n", r);
				return r;
			}

			break;

		default:
			if((r = sys_page_map(parent_envid, pgaddr, envid, pgaddr, PTE_P | PTE_U)) < 0){

				cprintf("duppage: sys_page_map return %e\n for RO map\n", r);
				return r;
			}
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t child_envid;
	uint32_t addr, addr_top;
	int r;

	// Set up our page fault handler appropriately.
	set_pgfault_handler(pgfault);

	// Create a child.
	if((child_envid = sys_exofork()) < 0)
		panic("fork: sys_exofork return %e\n", child_envid);

	if(child_envid == 0){

		thisenv = &envs[ENVX(sys_getenvid())];

		return 0;
	}

	// Copy our address space.
	addr_top = UXSTACKTOP - PGSIZE;

	for(addr = 0; addr < addr_top; addr += PGSIZE){
	
		if(!(uvpd[PDX(addr)] & PTE_P) || !(uvpt[PGNUM(addr)] & PTE_P))
			continue;

		if(duppage(child_envid, PGNUM(addr)) < 0)
			panic("fork: duppage fail for pte num %d\n", PGNUM(addr));
	}

	// Alloc exception-stack for the child so he can start run.
	if((r = sys_page_alloc(child_envid, (void*)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		panic("fork: sys_page_alloc return %e\n", r);

	if((r = sys_env_set_pgfault_upcall(child_envid, thisenv->env_pgfault_upcall)) < 0)
		panic("fork: sys_env_pgfault_upcall return %e\n", r);

	// Mark the child as runnable and return.
	if((r = sys_env_set_status(child_envid, ENV_RUNNABLE)) < 0)
		panic("fork: sys_env_set_status return %e\n", r);

	return child_envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
