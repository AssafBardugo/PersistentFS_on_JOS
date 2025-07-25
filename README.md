# JOS Persistent File System (PFS)
**Author:** Assaf Bardugo

> A versioned, time-traveling file system layer for JOS.

This project extends the JOS operating system’s file system with a **Persistent File System (PFS)** layer.  
Every mutation to a file creates a snapshot, stored in a hidden directory, allowing users to navigate and restore historical versions.


## Motivation
Most file systems overwrite data in-place, losing historical state.  
Inspired by persistent data structures, this PFS implementation enables file versioning.  
Each change creates a read-only snapshot, allowing inspection and rollback.

---

## Features
- **Hidden Fat File**  
  Each file includes a hidden directory (`FTYPE_FF`) that stores all of its snapshots.

- **Incremental Timestamps**  
  Snapshots are timestamped; a global `last_ts` is stored in the superblock and persisted on disk.

- **Efficient Storage**  
  Block pointers are reused between versions unless modified.

- **New Commands**  
  - `cd`, `mkdir`, `touch`
  - Append support via `>>` and `O_APPEND`
  - `track [-t] <file>` – list or restore versions  
  - `undo <file>` – revert to the previous version (shorthand for `track -t -1 <file>`)

- **Garbage Collector**  
  Helps keep the buffer cache clean and optimized.

---

## Usage

### Basic File Operations
All standard file operations (`create`, `open`, `read`, `write`) remain functional.  
Upon creation under PFS, a hidden fat file is initialized, storing the current timestamp.

### `track` Command
```sh
$ track myfile.txt           # List all timestamps and sizes
$ track -t 3 myfile.txt      # Restore version 3 as a new version
$ track -t -3 myfile.txt     # Restore (last_ts - 3) as a new version
