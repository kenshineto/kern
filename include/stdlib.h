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

#endif /* stlib.h */
