/**
 * @file stdio.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Standard I/O definitions.
 */

#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>

// TODO: implement
typedef void FILE;

extern FILE *stdin;
extern FILE *stdout;
#define stdin stdin
#define stdout stdout

/**
 * Prints out a char
 *
 * @param c - the char
 */
extern void putc(char c);

/**
 * Prints out a null terminated string
 *
 * @param s - the string
 */
extern void puts(const char *s);

/**
 * Prints out a char
 *
 * @param stream - stream to write to
 * @param c - the char
 */
extern void fputc(FILE *stream, char c);

/**
 * Prints out a null terminated string
 *
 * @param stream - stream to write to
 * @param s - the string
 */
extern void fputs(FILE *stream, const char *s);

/**
 * prints out a formatted string
 *
 * @param format - the format string
 * @param ... - variable args for the format
 */
__attribute__((format(printf, 1, 2))) extern void printf(const char *format,
														 ...);

/**
 * prints out a formatted string to a buffer
 *
 * @param s - the string to write to
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes written
 */
__attribute__((format(printf, 2, 3))) extern size_t
sprintf(char *restrict s, const char *format, ...);

/**
 * prints out a formatted string to a buffer with a given max length
 *
 * @param s - the string to write to
 * @param maxlen - the max len of the buffer
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes written
 */
__attribute__((format(printf, 3, 4))) extern size_t
snprintf(char *restrict s, size_t maxlen, const char *format, ...);

/**
 * prints out a formatted string
 *
 * @param format - the format string
 * @param args - variable arg list for the format
 */
extern void vprintf(const char *format, va_list args);

/**
 * prints out a formatted string to a buffer
 *
 * @param s - the string to write to
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes written
 */
extern size_t vsprintf(char *restrict s, const char *format, va_list args);

/**
 * prints out a formatted string to a buffer with a given max length
 *
 * @param s - the string to write to
 * @param maxlen - the max len of the buffer
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes written
 */
extern size_t vsnprintf(char *restrict s, size_t maxlen, const char *format,
						va_list args);

/**
 * prints out a formatted string
 *
 * @param stream - the opened stream to print to
 * @param format - the format string
 * @param ... - variable args for the format
 */
__attribute__((format(printf, 2, 3))) extern void
fprintf(FILE *stream, const char *format, ...);

/**
 * prints out a formatted string
 *
 * @param stream - the opened stream to print to
 * @param format - the format string
 * @param args - variable arg list for the format
 */
extern void vfprintf(FILE *stream, const char *format, va_list args);

/**
 * opens a file with a given file name
 *
 * @param filename - the name of the file to open
 * @returns the file pointer of success, NULL on error
 */
extern FILE *fopen(const char *filename);

/**
 * closes a opened file
 *
 * @param stream - the opened file stream
 */
extern void fclose(FILE *stream);

/**
 * reads data from a file into a pointer
 * @param ptr - the buffer to write into
 * @param size - the size of the block to read
 * @param n - the count of blocks to read
 * @param stream - the file stream to read from
 * @returns the number of blocks read
 */
extern size_t fread(void *ptr, size_t size, size_t n, FILE *stream);

/**
 * writes data from a pointer into a file
 * @param ptr - the buffer to read from
 * @param size - the size of the block to write
 * @param n - the count of blocks to write
 * @param stream - the file stream to write into
 * @returns the number of blocks written
 */
extern size_t fwrite(void *ptr, size_t size, size_t n, FILE *stream);

#endif /* stdio.h */
