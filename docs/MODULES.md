# modules

list of all kernel modules and their functions

## cpu/

Initalizes all cpu components and low level tabels.
- FPU / SSE / AVX
- IDT (Interrupt Descriptor Table)
- PIC (Programmable Interrupt Controller)
- TSS (Task State Segment)
  - used for allowing kernel to switch into ring 3
  - used for setting kernel stack on interrupt

## drivers/

Folder `drivers/` contains drivers for the system. See DRIVERS.md.

File `drivers.c` loads each of the drivers in `drivers/`.

## efi/

Load UEFI components
- UEFI memory map
- GOP (Graphics Output Protocol), UEFI framebuffer

## entry.S

Entry point into the kernel along with:
- GDT (Global Descriptor Table)
- Multiboot header see https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
- Inital kernel stack
  - not used once interrupts since interrupt stack will be used
- Inital page tables
  - see MEMORY.md

Symbols:
- `_start`
  - generic legacy bios entrypoint in IA-32 mode

- `_start_efi`
  - uefi entrypoint in IA-32e mode

## font/

Loads psf2 font files into kernel .rodata segment

## fs/

See FS.md

## include/

All kernel headers.

- SEE HEADERS.md

## input.c

Abstracts all input from all sources over a single keycode / mouseevent buffer.
- uses ps2 and uart

## lib/

Kernel c library

Notable files:

- `backtrace.c` - does stack backtraces and logs them to output
  - used during exceptions
- `kspin.c`
  - spinlock in kernel space
- `panic.c`
  - implements panic functions, along with halt called during exceptions or panic

## main.c

Kernel main entrypoint.
- Loads all components of the kernel in order.
- Loads init process
- Dispatches init

## mboot/

Laod information provided by multiboot standard
- `efi.c` - gets `EFI_SYSTEM_TABLE` & `EFI_HANDLE`
  - read UEFI standard for more information
- `mmap.c` - load memory map when in legacy boot
  - memory map is loaded from `efi/` module when booting from UEFI
- `module.c`
  - loads initrd (ramdisk)
- `rsdp.c`
  - load root ACPI table, provided for the ACPI driver

## memory/

See MEMORY.md

## procs.c

Stores loaded process information and scheduler
- multiple queues for scheduling
  - ready - pcb is ready to be executed
  - zombie - pcb is a zombie, and waiting to be cleaned up
  - syscall - replaced blocked/waiting in baseline
    - each syscall has its own queue
    - acessed though syscall_queue[SYS_num]

See PCB.md for pcb information.

# syscall.c

Syscall implentation functions for each syscall

See SYSCALLS.md

# term.c

Manages text terminal. All text printed to standard out or err will be printed
to this terminal.

- contains a terminal buffer for allowing scrolling of the screen
- manages redrawing the terminal when scrolling or changing terminal handler
  - vga text mode v.s. gpu framebuffer

# user.c

Loads user ELF binaries into memory and initalizes them into a PCB.

See PCB.md

- `user_load` - load a user process form the filesystem into memory
- `user_clone` - clones the user process
- `user_cleanup` - cleans up a user process

