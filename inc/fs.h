// See COPYRIGHT for copyright information.

#ifndef JOS_INC_FS_H
#define JOS_INC_FS_H

#include <inc/types.h>
#include <inc/mmu.h>

// File nodes (both in-memory and on-disk)

// Bytes per file system block - same as page size
#define BLKSIZE		PGSIZE
#define BLKBITSIZE	(BLKSIZE * 8)

// Maximum size of a filename (a single path component), including null
// Must be a multiple of 4
#define MAXNAMELEN	128

// Maximum size of a complete pathname, including null
#define MAXPATHLEN	1024

// Number of block pointers in a File descriptor
#define NDIRECT		10
// Number of direct block pointers in an indirect block
#define NINDIRECT	(BLKSIZE / 4)

#define MAXFILESIZE	((NDIRECT + NINDIRECT) * BLKSIZE)

typedef uint32_t ts_t;	// PROJECT

// Global last timestamp
ts_t last_ts;	// PROJECT 

// Global timestamp for opening fat files. 
// The user can change it via the sys_set_track_ts system call to open a file at a specific time in the past. 
// Its default value is last_ts.
ts_t track_ts;	// PROJECT

// Flag -a of command ls
// It's global because functions in fs/fs.c use it to know how to treat ff files
bool ls_all;	// PROJECT

// PROJECT:
// When File.f_type is FTYPE_FN it's mean all the blocks of the File 
// containing versions of the file. Each for every timestamp.
struct File {
	char f_name[MAXNAMELEN];	// filename
	off_t f_size;			// file size in bytes
	uint32_t f_type;		// file type
	ts_t f_timestamp;		// PROJECT: last timestamp of the file

	// Block pointers.
	// A block is allocated iff its value is != 0.
	uint32_t f_direct[NDIRECT];	// direct blocks
	uint32_t f_indirect;		// indirect block

	// Pad out to 256 bytes; must do arithmetic in case we're compiling
	// fsformat on a 64-bit machine.
	uint8_t f_pad[256 - MAXNAMELEN - 12 - 4*NDIRECT - 4];	// PROJECT: Changed from -8 to -12
} __attribute__((packed));	// required only on some 64-bit machines

// An inode block contains exactly BLKFILES 'struct File's
#define BLKFILES	(BLKSIZE / sizeof(struct File))


// File types
#define FTYPE_REG	0	// Regular file
#define FTYPE_DIR	1	// Directory
#define FTYPE_FF	2	// Fat File 	// PROJECT


// File system super-block (both in-memory and on-disk)

#define FS_MAGIC	0x4A0530AE	// related vaguely to 'J\0S!'

struct Super {
	uint32_t s_magic;		// Magic number: FS_MAGIC
	uint32_t s_nblocks;		// Total number of blocks on disk
	ts_t last_ts;			// PROJECT: save global timestamp on-disk!
	struct File s_root;		// Root directory node
};

// Definitions for requests from clients to file system
enum {
	FSREQ_OPEN = 1,
	FSREQ_SET_SIZE,
	// Read returns a Fsret_read on the request page
	FSREQ_READ,
	FSREQ_WRITE,
	// Stat returns a Fsret_stat on the request page
	FSREQ_STAT,
	FSREQ_FLUSH,
	FSREQ_REMOVE,
	FSREQ_SYNC
};

union Fsipc {
	struct Fsreq_open {
		char req_path[MAXPATHLEN];
		int req_omode;
	} open;

	struct Fsreq_set_size {
		int req_fileid;
		off_t req_size;
	} set_size;

	struct Fsreq_read {
		int req_fileid;
		size_t req_n;
	} read;

	struct Fsret_read {
		char ret_buf[PGSIZE];
	} readRet;

	struct Fsreq_write {
		int req_fileid;
		size_t req_n;
		char req_buf[PGSIZE - (sizeof(int) + sizeof(size_t))];
	} write;

	struct Fsreq_stat {
		int req_fileid;
	} stat;

	struct Fsret_stat {
		char ret_name[MAXNAMELEN];
		off_t ret_size;
		int ret_ftype;	// PROJECT
	} statRet;

	struct Fsreq_flush {
		int req_fileid;
	} flush;

	struct Fsreq_remove {
		char req_path[MAXPATHLEN];
	} remove;

	// Ensure Fsipc is one page
	char _pad[PGSIZE];
};

#endif /* !JOS_INC_FS_H */
