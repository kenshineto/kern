/**
 * @file limits.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Defined kernel limits
 */

/// number of pts to identity map the kernel (1pt = 2MB)
#define N_IDENT_PTS 4 // max 512 (1G)

/// max number of processes
#define N_PROCS 256

/// max nubmer of pci devices
#define N_PCI_DEV 256

/// max memory entires
#define N_MMAP_ENTRY 256

/// max fs limits
#define N_FILE_NAME 256
#define N_DIR_ENTS 256
#define N_DISKS 8

/// elf limits
#define N_ELF_SEGMENTS 16

/// input buffer
#define N_KEYCODE 64
#define N_MOUSEEV 64

/// length of terminal buffer
#define TERM_MAX_WIDTH 1920
#define TERM_MAX_HEIGHT 1080
