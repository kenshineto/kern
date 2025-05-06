# PCB

PCB information

## context

Contains context infromation for the curernt process.
- memory context (pcb->memctx)
  - stores page directory and vitural address allocateor (see MEMORY.md)
- context save area (pcb->regs)

## medatada

- `pid` - process idenfitication number
- `parent` ` - parent of the current process
  - can be NULL if the init process
- `state` - the current runing state of the process
  - `UNUSED` - proces in ptable is not used
  - `NEW` - process in ptable has been allocated but has not been initalized
  - `READY` - process is ready to be dispatched (and in ready queue)
  - `RUNNING` - process is the current running process (and in no queues)
  - `BLOCKED` - process is in a syscall queue waiting on their syscall to return
  - `ZOMBIE` - process is a zombie and waiting to be cleaned up (in zombie queue)
- `priority` - running process priority
  - any number form 0 to SIZET_MAX
  - higher priority means longer wait to be scheduled
- `ticks` - number of ticks the process has been running for
  - set to zero when process is not running

## heap

- `heap_start` is the start of the progams heap
- `heap_len` is the length of the programs heap

## open files

- `open_files` is a list of currently opened files indexed by file descriptor

## elf metadata

- `elf_header` - the programs elf header (Elf64_Ehdr)
- `elf_segments` - the programs loadable elf segments (Elf64_Phdr[])
- `n_elf_segments` - the number of elf segmsnts the program has

## queue linkage

- `next` - the next pcb in the current queue this pcb is in

## processs state information

- `syscall` - the current syscall this process is blocked on
- `wakeup` - the number of ticks until the process can be waked up (used during SYS_sleep)
- `exit_status` - the exit status of the process when a zombie
