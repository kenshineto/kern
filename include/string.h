/**
 * @file string.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * String libaray functions
 */

#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

/**
 * Compare the first n bytes (interpreted as unsigned char) of the memory areas s1 and s2
 * @param s1 - the first memory area
 * @param s2 - the second memory area
 * @param n - the byte count
 * @returns an interger less than, equal to, or greater than 0 if the first n bytes
 * of s1 are less than, equal to, or greater than s2.
 */
extern int memcmp(const void *restrict s1, const void *restrict s2, size_t n);

/**
 * Copy the first n bytes from memory area src to memory area dest. The memory
 * areas must not overlap.
 * @param dest - the destination
 * @param src - the source
 * @param n - the byte count
 * @returns a pointer to dest
 */
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);

/**
 * Copy the first n bytes from memory area src to memory area dest. The memory
 * areas may overlap; memmove behaves as though the bytes are first copied to a
 * temporary array.
 * @param dest - the destination
 * @param src - the source
 * @param n - the byte count
 * @returns a pointer to dest
 */
extern void *memmove(void *restrict dest, const void *restrict src, size_t n);

/**
 * Fill the first n bytes of the memory region dest with the constant byte c.
 * @param dest - the destination
 * @param c - the byte to write
 * @param n - the byte count
 * @returns a pointer to dest
 */
extern void *memset(void *restrict dest, int c, size_t n);

/**
 * Copy the first n bytes from memory area src to memory area dest. The memory
 * areas must not overlap.
 * @param dest - the destination
 * @param src - the source
 * @param n - the byte count
 * @returns a pointer to dest
 */
extern void *memcpyv(volatile void *restrict dest,
					 const volatile void *restrict src, size_t n);

/**
 * Copy the first n bytes from memory area src to memory area dest. The memory
 * areas may overlap; memmove behaves as though the bytes are first copied to a
 * temporary array.
 * @param dest - the destination
 * @param src - the source
 * @param n - the byte count
 * @returns a pointer to dest
 */
extern void *memmovev(volatile void *restrict dest,
					  const volatile void *restrict src, size_t n);

/**
 * Fill the first n bytes of the memory region dest with the constant byte c.
 * @param dest - the destination
 * @param c - the byte to write
 * @param n - the byte count
 * @returns a pointer to dest
 */
extern void *memsetv(volatile void *restrict dest, int c, size_t n);

/**
 * Calculates the length of the string pointed to by str, excluding
 * the terminating null byte
 * @param str - the string pointer
 * @returns the length of the string in bytes
 */
extern size_t strlen(const char *str);

/**
 * Compare null terminated string s1 and s2. The comparison is done using
 * unsigned characters.
 * @param s1 - a pointer to the first string
 * @param s2 - a pointer to the second string
 * @returns an interger less than, equal to, or greater than 0 if s1 compares less
 * than, equal to, or greater than s2
 */
extern int strcmp(const char *restrict s1, const char *restrict s2, size_t n);

/**
 * Compare at most the first n bytes of the strings s1 and s2. The comparison is
 * done using unsigned characters.
 * @param s1 - a pointer to the first string
 * @param s2 - a pointer to the second string
 * @param n - the maximum number of bytes
 * @returns an interger less than, equal to, or greater than 0 if s1 compares less
 * than, equal to, or greater than s2
 */
extern int strncmp(const char *restrict s1, const char *restrict s2, size_t n);

/**
 * Copies the string pointed to by src into the buffer pointer to by dest.
 * The dest buffer must be long enough to hold src.
 * @param dest - the destination
 * @param src - the source
 * @returns a pointer to dest
 */
extern char *strcpy(char *restrict dest, const char *restrict src);

/**
 * Copies the string pointed to by src into the buffer pointer to by dest.
 * The dest buffer must be long enough to hold src or size n.
 * @param dest - the destination
 * @param src - the source
 * @param n - the maximum number of bytes
 * @returns a pointer to dest
 */
extern char *strncpy(char *restrict dest, const char *restrict src, size_t n);

/**
 * Copies the string pointed to by src into the buffer pointed to by dest.
 * The dest buffer must be long enough to hold src.
 * @param dest - the destination
 * @param src - the source
 * @param n - the maximum number of bytes
 * @returns a pointer to the terminating null byte
 */
extern char *stpcpy(char *restrict dest, const char *restrict src);

/**
 * Copies the string pointed to by src into the buffer pointed to by dest.
 * The dest buffer must be long enough to hold src or size n.
 * @param dest - the destination
 * @param src - the source
 * @param n - the maximum number of bytes
 * @returns a pointer to the byte after the last character copied
 */
extern char *stpncpy(char *restrict dest, const char *restrict src, size_t n);

#endif /* string.h */
