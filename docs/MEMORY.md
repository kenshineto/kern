# Memory

Comus (the kernel) is a x86_64 arch kernel using paging (vitural memory).

## Vitural Memory

Vitural memory works by using vitural address instead of direct physical addresses
when accessing memory. These vitural address are then translated into
physical addresses using a set of structures called "page tables".

Comus uses for level paging using the following page structures:
- PML4 (level 4)
- PDPT (level 3)
- PD (level 2)
- PT (level 1)

Read intel / amd manual for more information.

## Memory Layout

|----------------|----------------|
| Start Address  | Data           |
|----------------|----------------|
| 0MB            | BIOS Reserved  |
| 1MB            | Kernel Start   |
| 129MB          | Kernel End     |
| ...            | User Program   |
| ...            | User Heap      |
| 0x7fffffffffff | User Stack Top |
|----------------|----------------|

## Memory Modules

To be able to map physical memory to vitural addresses, the following modules
are needed (and used).

### Phyiscal Page Allocator

Marks specific physical pages as in used or free to be used. Paging functions
request phyiscal pages when allocating memory.

Allocator implemented as a bitmap. Bitmap is a list of uint64_t, where each
page is marged as a single bit
  - 0 for free
  - 1 for in use

Which pages the bitmap points to is initalized using the memory map provided
by multiboot or UEFI.

## Vitural Address Allocator

When attempting to map memory, its important for vitural addresses to not be
reused multiple times. This allocate gurenteed this doesnt happend for any
given memory context.

The allocator is implemented a linked list of free / in use nodes (blocks) of
vitural addresss. At the start is a single node describing the entire vitural
address space. To mark an address block as inuse, the node is split, marking
new new block as in use, and the other new nodes next to it as not in use.

### virtaddr_alloc
If the kernel wants to map memory at any (random) vitural address, the allocator
will return the first free contigious vitural address it finds.

### virtaddr_take
If the kernel wants to map a specific vitural address, this allocator will
either state that its taken (and cannot be used), or state that it can be used
marking it as in use in the allocator.

## Bootstrap Page Tables
To successfully identity map the kernel, some memory needs to be allocated
inside the kernel. This is because only mapped memory can be written to.
If its not in the kernel, this memory will not be identity mapped, this making
it inaccessable.

These bootstrap page tables are used for creating the kernel identiy mapping.

## Paging Page Tables (bad name)
These page tables (mapped by the bootstrap page tables) are used for mapping
other page table structures to be accessable. These page tables are inside the
kernel and are this identity mapped, therefore always accessable. They allow
mapping any address, but again are primarly used for mapping other page tables.

## Paging Functions

The following functions used to be able to map pages
- map_pages
  - maps a set of pages at a phys/virt address pair
- unmap_pages
  - the reverse of map_pages
- mem_map_pages_at
  - allocate physical pages and map them at a vitural address
  - used for normal allocations
- mem_map_addr
  - map a vitural address (or random one if not provided) to a given physical address
  - used for accessing memory when provided with a physical address
    - ACPI tables, ramdisk
