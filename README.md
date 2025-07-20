Assaf Bardugo

JOS Persistent File System (PFS)

This project extends the JOS operating system’s File System (FS) with a Persistent File System (PFS) layer. 
PFS preserves every historical version of a file by creating a hidden “fat file” directory for each file; every mutation spawns a new version tagged by an incrementing timestamp. 
Users can navigate, inspect, and restore previous versions using the provided track command.

---
Table of Contents
1. Motivation
2. Features
4. Usage
   Basic File Operations
   track Command
   Time-Travel via touch and >>
5. Implementation Details
   Superblock & Global Timestamp
   Fat File Structure
   Modifications to fs.c & fsformat.c
   Shared Path for Shell Built-ins
   New Syscalls
6. File Layout
---

Motivation
Typical file systems overwrite data in-place, losing historical state. Inspired by persistent data structures, this PFS extension enables time-travel: every change spawns a new read-only snapshot of a file, so users can review or roll back to older versions.

Features
- Hidden Fat File: A per-file hidden directory (new file type: FTYPE_FF) storing all versions.
- Incremental Timestamps: timestamp in each File record; global last_ts in the superblock that saved on the disk.
- Space Efficiency: Block pointers are shared between snapshots unless modified.
- User New Commands:
  - cd, mkdir and touch.
  - Append operator >> and O_APPEND flag.
  - track [-t] <file>: list all versions or restore past versions by specify timestamp.
  - undo <file>: restore from one version before (a short for track -t -1 <file>).
- Garbage Collector is helping keep buffer cache clean.

Usage
Basic File Operations
All file operations (create, open, read, write) work as before. When a file is first created under PFS, a hidden fat file is created, and current timestamp is stored.

Time-Travel via track Command
$ track myfile.txt         	# Lists all timestamps and their sizes
$ track -t 3 myfile.txt    	# Restore the version 3 of myfile.txt as a new version
$ track -t -3 myfile.txt    	# Restore the version last_ts-3 of myfile.txt as a new version

touch and >>
- touch myfile.txt create the fatfile with first initial version.
- The >> operator automatically creates a new snapshot: each append increments the global timestamp and clones only the metadata pointers.

Implementation Details
Superblock & Global Timestamp
- Added size_t last_ts; to struct Super in inc/fs.h (loaded from disk in fs_init()).
- Each new snapshot increments super.last_ts and is persisted on-disk.

Fat File Structure
- Defined #define FTYPE_FF 0x10 for fat file entries.
- A fat file is a hidden directory; inside, each snapshot is stored as a regular file named by its timestamp.
- Metadata-only duplication: only block pointers are copied on snapshot creation to save space.

Modifications to fs.c & fsformat.c
- fsformat.c: create root-level pfs/ directory on formatting, add first version 0.
- fs.c:
  - ff_lookup(path, timestamp, &fileid) to retrieve a version; helps walk_path to navigate into fat file directories.
  - Update file_create to initialize fat files for new creations.
  - file_shalldup() clones metadata for writes under PFS.

Shared Path for Shell Built-ins
- Introduced a shared page at PATH_VA (in inc/lib.h) to maintain the current working directory across shell child processes.
- Modified sh.c, cd.c, and ls.c to support PATH navigation.

File Layout
inc/fs.h          # New macros, and shared PATH_VA
fs/fsformat.c     # Superblock initialization and root pfs/ setup
fs/fs.c           # Core PFS logic: lookup, dup, create
fs/serv.c         # update serve_open with two modes for walk_path: WALK_RDONLY, WALK_CREAT
user/sh.c, cd.c   # Shell updates for shared PATH and >> operator
user/ls.c         # Listing ff entries and --all behavior

