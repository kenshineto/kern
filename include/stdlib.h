/**
 * @file stdlib.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Standard libaray functions
 */

#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define PAGE_SIZE 4096

/**
 * converts single digit int to base 36
 * @param i - int
 * @returns c - base 36 char
 */
extern char itoc(int i);

/**
 * converts single base 36 chat into int
 * @param c - base 36 char
 * @returns i - int, or -1 if the char was invalid
 */
extern int ctoi(char c);

/**
 * Converts the initial portiion of the string pointed to by s to int.
 * @param s - the string to convert
 * @returns the number inside s or 0 on error
 */
extern int atoi(const char *s);

/**
 * Converts the initial portiion of the string pointed to by s to long.
 * @param s - the string to convert
 * @returns the number inside s or 0 on error
 */
extern long int atol(const char *s);

/**
 * Converts the initial portiion of the string pointed to by s to long long.
 * @param s - the string to convert
 * @returns the number inside s or 0 on error
 */
extern long long int atoll(const char *s);

/**
 * Converts a integer to asci inside a string with a given radix (base).
 * @param n - the number to convert
 * @param buffer - the string buffer
 * @param radix - the base to convert
 */
extern char *itoa(int n, char *buffer, int radix);

/**
 * Converts a long to asci inside a string with a given radix (base).
 * @param n - the number to convert
 * @param buffer - the string buffer
 * @param radix - the base to convert
 */
extern char *ltoa(long int n, char *buffer, int radix);

/**
 * Converts a long long to asci inside a string with a given radix (base).
 * @param n - the number to conver
 * @param buffer - the string buffer
 * @param radix - the base to convert
 */
extern char *lltoa(long long int n, char *buffer, int radix);

/**
 * Converts a unsigned integer to asci inside a string with a given radix (base).
 * @param n - the number to convert
 * @param buffer - the string buffer
 * @param radix - the base to convert
 */
extern char *utoa(unsigned int n, char *buffer, int radix);

/**
 * Converts a unsigned long to asci inside a string with a given radix (base).
 * @param n - the number to convert
 * @param buffer - the string buffer
 * @param radix - the base to convert
 */
extern char *ultoa(unsigned long int n, char *buffer, int radix);

/**
 * Converts a unsigned long long to asci inside a string with a given radix (base).
 * @param n - the number to conver
 * @param buffer - the string buffer
 * @param radix - the base to convert
 */
extern char *ulltoa(unsigned long long int n, char *buffer, int radix);

/**
 * Converts the string in str to an int value based on the given base.
 * The endptr is updated to where the string was no longer valid.
 * @param str - the string buffer
 * @param endptr - the endptr
 * @param base - the base to convert to
 * @returns 0 on error or success, error if endptr is still equal to str
 */
extern int strtoi(const char *str, char **endptr, int base);

/**
 * Converts the string in str to a long value based on the given base.
 * The endptr is updated to where the string was no longer valid.
 * @param str - the string buffer
 * @param endptr - the endptr
 * @param base - the base to convert to
 * @returns 0 on error or success, error if endptr is still equal to str
 */
extern long int strtol(const char *str, char **endptr, int base);

/**
 * Converts the string in str to a long long value based on the given base.
 * The endptr is updated to where the string was no longer valid.
 * @param str - the string buffer
 * @param endptr - the endptr
 * @param base - the base to convert to
 * @returns 0 on error or success, error if endptr is still equal to str
 */
extern long long int strtoll(const char *str, char **endptr, int base);

/**
 * Converts the string in str to an unsigned int value based on the given base.
 * The endptr is updated to where the string was no longer valid.
 * @param str - the string buffer
 * @param endptr - the endptr
 * @param base - the base to convert to
 * @returns 0 on error or success, error if endptr is still equal to str
 */
extern unsigned int strtoui(const char *str, char **endptr, int base);

/**
 * Converts the string in str to an unsigned long value based on the given base.
 * The endptr is updated to where the string was no longer valid.
 * @param str - the string buffer
 * @param endptr - the endptr
 * @param base - the base to convert to
 * @returns 0 on error or success, error if endptr is still equal to str
 */
extern unsigned long int strtoul(const char *str, char **endptr, int base);

/**
 * Converts the string in str to an unsigned long long value based on the given base.
 * The endptr is updated to where the string was no longer valid.
 * @param str - the string buffer
 * @param endptr - the endptr
 * @param base - the base to convert to
 * @returns 0 on error or success, error if endptr is still equal to str
 */
extern unsigned long long int strtoull(const char *str, char **endptr,
									   int base);

/**
 * Converts a byte count to a human readable file size of at most four characters
 * using binary suffixes.
 *
 * The following rules are applied:
 * - If the byte count is less than 1024, the count is written in decimal
 *   and no suffix is applied
 * - Otherwise, repeatedly divide by 1024 until the value is under 1000.
 * - If the value has two or three decimal digits, print it followed by the
 *   approprate suffix.
 * - If the value has one decimal digit, print it along with a single fractional
 *   digit. This also applies if the value is zero.
 *
 * @param bytes - the bytes to convert
 * @param buf - a pointer to the buffer to store it in (which must be at least five
 *              bytes long)
 * @returns - buf
 */
extern char *btoa(size_t bytes, char *buf);

/**
 * This function confines an argument within specified bounds.
 *
 * @param min - lower bound
 * @param value - value to be constrained
 * @param max - upper bound
 *
 * @returns the constrained value
 */
extern unsigned int bound(unsigned int min, unsigned int value,
						  unsigned int max);

/**
 * Delays the current process by spining the cpu
 *
 * @param count how longish to spin
 */
extern void delay(int count);

/**
 * Allocates size_t bytes in memory
 *
 * @param size - the amount of bytes to allocate
 * @returns the address allocated or NULL on failure
 */
extern void *malloc(size_t size);

/**
 * Rellocates a given allocated ptr to a new size of bytes in memory.
 * If ptr is NULL it will allocate new memory.
 *
 * @param ptr - the pointer to reallocate
 * @param size - the amount of bytes to reallocate to
 * @returns the address allocated or NULL on failure
 */
extern void *realloc(void *ptr, size_t size);

/**
 * Frees an allocated pointer in memory
 *
 * @param ptr - the pointer to free
 */
extern void free(void *ptr);

/**
 * Allocate a single page of memory
 *
 * @returns the address allocated or NULL on failure
 */
extern void *alloc_page(void);

/**
 * Allocate size_t amount of contiguous virtual pages
 *
 * @param count - the number of pages to allocate
 * @returns the address allocated or NULL on failure
 */
extern void *alloc_pages(size_t count);

/**
 * Free allocated pages.
 *
 * @param ptr - the pointer provided by alloc_page or alloc_pages
 */
extern void free_pages(void *ptr);

/**
 * Abort the current process with a given message.
 *
 * @param format - the format string
 * @param ... - variable args for the format
 */
__attribute__((noreturn)) extern void panic(const char *format, ...);

#endif /* stlib.h */
