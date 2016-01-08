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
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((err & FEC_WR) == 0){
		panic("pgfault err not FEC_WR\n");
	} else if ((vpd[VPD(addr)] & PTE_P) == 0){
		panic("pgfault vpd[VPD(addr)] not present\n");
	} else if ((vpt[VPN(addr)] & PTE_COW) == 0){
		panic("pgfault vpt[VPN(addr)] not COW\n");	
	}
	// if ((err & FEC_WR) == 0 || (vpd[VPD(addr)] & PTE_P) == 0 || (vpt[VPN(addr)] & PTE_COW) == 0){
	// 	panic("pgfault not a write or page table entry not exist or not a copy-on-write fault\n");
	// }
	// cprintf("pgfault a write or a COW page\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.
	
	// LAB 4: Your code here.
	if ((r = sys_page_alloc(0, (void *)PFTEMP, PTE_U | PTE_P | PTE_W)) < 0){
		panic("pgfault page allocation failed, returned %d\n", r);
	}
	// cprintf("pgfault page allocation succeeded\n");

	addr = ROUNDDOWN(addr, PGSIZE);

	memmove(PFTEMP, addr, PGSIZE);

	if ((r = sys_page_map(0, PFTEMP, 0, addr, PTE_U | PTE_P | PTE_W)) < 0){
		panic("pgfault page mapping failed, returned %d\n", r);
	}
	// cprintf("pgfault sys_page_map succeeded\n");
	
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why mark ours copy-on-write again
// if it was already copy-on-write?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
// 
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void *addr;
	pte_t pte;

	// LAB 4: Your code here.
	addr = (void *)((uint32_t)pn*PGSIZE);
	pte = vpt[VPN(addr)];

	if ((pte & PTE_W) > 0 || (pte & PTE_COW) > 0){
		if ((r = sys_page_map(0, addr, envid, addr, PTE_U | PTE_P | PTE_COW)) < 0){
			panic("duppage mapping parent->child failed. returned %d\n", r);
		}
		if ((r = sys_page_map(0, addr, 0, addr, PTE_U | PTE_P | PTE_COW)) < 0){
			panic("duppage re-mapping parent->parent with COW failed. returned %d\n", r);
		}
	} else {
		if ((r = sys_page_map(0, addr, envid, addr, PTE_U | PTE_P)) < 0){
			panic("duppage re-mapping parent->parent without COW failed. returned %d\n", r);
		}
	}
	// cprintf("duppage succeeded\n");
	// panic("duppage not implemented");
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
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" and the user exception stack in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);

	envid_t envid;
	uint32_t addr;
	int r;

	envid = sys_exofork();

	if (envid < 0){
		panic("sys_exofork failed, returned %d\n", envid);
	}

	// child
	if (envid == 0){
		cprintf("fork envid = 0\n");
		env = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// cprintf("fork envid = %d\n", envid & 0x3ff);

	// parent
	// space above UTOP(UXSTACKTOP) already mapped as the same as the kernel
	// between (UXSTACKTOP-PGSIZE, UXSTACKTOP) is User Exception Stack, needs page allocation 
	// between (UTEXT, UXSTACKTOP-PGSIZE) needs mapping
	for (addr = UTEXT; addr < UXSTACKTOP - PGSIZE; addr += PGSIZE){
		if ((vpd[VPD(addr)] & PTE_P) > 0 && (vpt[VPN(addr)] & PTE_P) > 0 && (vpt[VPN(addr)] & PTE_U) > 0){
			duppage(envid, VPN(addr));
		}
	}
	// alloc page between (UXSTACKTOP-PGSIZE)
	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)) < 0){
		panic("fork sys_page_alloc failed. returned %d\n", r);
	}

	extern void _pgfault_upcall(void);

	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);

	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0){
		panic("fork set_status failed. returned %d\n", r);
	}

	return envid;

	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
