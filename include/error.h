/**
 * @file errno.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Error codes.
 */

// public error codes
#define SUCCESS (0)
#define E_SUCCESS SUCCESS
#define E_FAILURE (-1)
#define E_BAD_PARAM (-2)
#define E_BAD_FD (-3)
#define E_NO_CHILDREN (-4)
#define E_NO_MEMORY (-5)
#define E_NOT_FOUND (-6)
#define E_NO_PROCS (-7)

// internal error codes
#define E_EMPTY_QUEUE (-100)
#define E_NO_PCBS (-101)
#define E_NO_PTE (-102)
#define E_LOAD_LIMIT (-103)

// exit status values
#define EXIT_SUCCESS (0)
#define EXIT_FAILURE (-1)
#define EXIT_KILLED (-101)
#define EXIT_BAD_SYSCALL (-102)
