
#include "fs.h"

// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P);
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (uvpt[PGNUM(va)] & PTE_D) != 0;
}

// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int r;

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	// Hint: first round addr to page boundary. fs/ide.c has code to read
	// the disk.
	//
	// LAB 5: you code here:
	addr = ROUNDDOWN(addr, PGSIZE);
	
	if((r = sys_page_alloc(0, addr, PTE_P | PTE_W | PTE_U)) < 0)
		panic("bc_pgfault: sys_page_alloc return %e\n", r);

	if((r = ide_read(blockno * BLKSECTS, addr, BLKSECTS)) < 0)
		panic("bc_pgfault: ide_read return %e\n", r);


	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	if ((r = sys_page_map(0, addr, 0, addr, uvpt[PGNUM(addr)] & PTE_SYSCALL)) < 0)
		panic("in bc_pgfault, sys_page_map: %e", r);

	// Check that the block we read was allocated. (exercise for
	// the reader: why do we do this *after* reading the block
	// in?)
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
// Hint: Use va_is_mapped, va_is_dirty, and ide_write.
// Hint: Use the PTE_SYSCALL constant when calling sys_page_map.
// Hint: Don't forget to round addr down.
void
flush_block(void *addr)
{
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);

	// LAB 5: Your code here.
	int r;

	addr = ROUNDDOWN(addr, PGSIZE);

	if(!va_is_mapped(addr) || !va_is_dirty(addr))
		return;

	if((r = ide_write(blockno * BLKSECTS, addr, BLKSECTS)) < 0)
		panic("flush_block: ide_write return %e\n", r);

	if((r = sys_page_map(0, addr, 0, addr, uvpt[PGNUM(addr)] & PTE_SYSCALL)) < 0)
		panic("flush_block: sys_page_map return %e", r);
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);

	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	struct Super super;
	set_pgfault_handler(bc_pgfault);
	check_bc();

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof super);
}


#define DEBUG_GC	0

/****** Challenge: ******/
// This function is called from fs/serv.c:359
//
// The purpose of this function is to prevent an "out of memory" situation due to the cache, 
// by clearing blocks from the cache if they have not been used for a long time.
void
garbage_collector(void)
{
	envid_t curenv_id = 0;
	uint32_t blockno;
	void* addr;
	bool page_was_accessed;
	int r;
	uint32_t nbitblocks;

#if DEBUG_GC
	static uint8_t blkctr[1024] = {0};
	static int call_ctr;
	static int blocks_removed_ctr;
	++call_ctr;
#endif

	// We want to make sure we don't unmap a bitmap block.
	nbitblocks = (super->s_nblocks + BLKBITSIZE - 1) / BLKBITSIZE;

	// Go through all the blocks, 
	// if you find a block that has not been accessed since 
	// the last time this function was called, 
	// write it to disk (if necessary) and return the page to the free_list.
	for(blockno = 2 + nbitblocks; blockno < super->s_nblocks; ++blockno){

		// If the block is free, continue.
		if(bitmap[blockno / 32] & (1 << (blockno % 32)))
			continue;

		addr = diskaddr(blockno);

		// If the block is not yet mapped, continue.
		if(!(uvpd[PDX(addr)] & PTE_P) || !(uvpt[PGNUM(addr)] & PTE_P))
			continue;

		page_was_accessed = uvpt[PGNUM(addr)] & PTE_A;

		// Flush the contents of the block out to disk if necessary, 
		// and clear the Dirty and Accessed bits using sys_page_map.
		// So that next time we can check if it turns back on.
		flush_block(addr);

		if(!page_was_accessed)
			continue;

		// sys_page_unmap will call page_remove
		if((r = sys_page_unmap(curenv_id, addr)) < 0)
			panic("garbage_collector: sys_page_unmap return %e for addr %08x\n", r, addr);

		// We do not mark the block as free because from the file system's perspective the block is still in use.	

#if DEBUG_GC
		++blkctr[blockno];
		++blocks_removed_ctr;
#endif
	}

#if DEBUG_GC
	cprintf("DEBUG: GC called for the %d time and remove so far %d blocks\n", call_ctr, blocks_removed_ctr);

	cprintf("GC statistic:\n");
	for(blockno = 3; blockno < 1024; ++blockno){

		if(blkctr[blockno] > 0)
			cprintf("block %d removed %d times\n", blockno, blkctr[blockno]);
	}
#endif
}

