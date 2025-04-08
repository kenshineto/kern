/**
 * @file kio.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Kernel I/O definitions.
 */

#ifndef _KIO_H
#define _KIO_H

#include <stddef.h>
#include <stdarg.h>

/**
 * Prints out a char
 *
 * @param c - the char
 */
void kputc(char c);

/**
 * Prints out a null terminated string
 *
 * @param s - the string
 */
void kputs(const char *s);

/**
 * prints out a formatted string
 *
 * @param format - the format string
 * @param ... - variable args for the format
 */
__attribute__((format(printf, 1, 2))) void kprintf(const char *format, ...);

/**
 * prints out a formatted string to a buffer
 *
 * @param s - the string to write to
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes written
 */
__attribute__((format(printf, 2, 3))) size_t ksprintf(char *restrict s,
													  const char *format, ...);

/**
 * prints out a formatted string to a buffer with a given max length
 *
 * @param s - the string to write to
 * @param maxlen - the max len of the buffer
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes written
 */
__attribute__((format(printf, 3, 4))) size_t ksnprintf(char *restrict s,
													   size_t maxlen,
													   const char *format, ...);

/**
 * prints out a formatted string
 *
 * @param format - the format string
 * @param args - variable arg list for the format
 */
void kvprintf(const char *format, va_list args);

/**
 * prints out a formatted string to a buffer
 *
 * @param s - the string to write to
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes written
 */
size_t kvsprintf(char *restrict s, const char *format, va_list args);

/**
 * prints out a formatted string to a buffer with a given max length
 *
 * @param s - the string to write to
 * @param maxlen - the max len of the buffer
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes written
 */
size_t kvsnprintf(char *restrict s, size_t maxlen, const char *format,
				  va_list args);

#endif /* kio.h */
