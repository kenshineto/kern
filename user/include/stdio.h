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

#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

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
 * @returns the character written or EOF on failure
 */
extern int putchar(int c);

/**
 * Prints out a char to a stream
 *
 * @param c - the char
 * @param stream - the stream to print to
 * @returns the character written or EOF on failure
 */
extern int putc(int c, FILE *stream);

/**
 * Prints out a char to a stream
 *
 * @param c - the char
 * @param stream - stream to write to
 * @returns the character written or EOF on failure
 */
extern int fputc(int c, FILE *stream);

/**
 * Prints out a null terminated string with newline
 *
 * @param str - the string
 * @returns nonnegative integer on success, EOF on error
 */
extern int puts(const char *str);

/**
 * Prints out a null terminated string
 *
 * @param str - the string
 * @param stream - stream to write to
 * @returns nonnegative integer on success, EOF on error
 */
extern int fputs(const char *str, FILE *stream);

/**
 * prints out a formatted string
 *
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes written
 */
__attribute__((format(printf, 1, 2))) extern int printf(const char *format,
														...);

/**
 * prints out a formatted string to a buffer
 *
 * @param s - the string to write to
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes written
 */
__attribute__((format(printf, 2, 3))) extern int
sprintf(char *restrict s, const char *format, ...);

/**
 * prints out a formatted string to a buffer with a given max length
 *
 * @param s - the string to write to
 * @param maxlen - the max len of the buffer
 * @param format - the format string
 * @param ... - variable args for the format
 * @returns number of bytes that would of been written (past maxlen)
 */
__attribute__((format(printf, 3, 4))) extern int
snprintf(char *restrict s, size_t maxlen, const char *format, ...);

/**
 * prints out a formatted string
 *
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes written
 */
extern int vprintf(const char *format, va_list args);

/**
 * prints out a formatted string to a buffer
 *
 * @param s - the string to write to
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes written
 */
extern int vsprintf(char *restrict s, const char *format, va_list args);

/**
 * prints out a formatted string to a buffer with a given max length
 *
 * @param s - the string to write to
 * @param maxlen - the max len of the buffer
 * @param format - the format string
 * @param args - variable arg list for the format
 * @returns number of bytes that would of been written (past maxlen)
 */
extern int vsnprintf(char *restrict s, size_t maxlen, const char *format,
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
 * @param modes - the modes to open the file in
 * @returns the file pointer of success, NULL on error
 */
extern FILE *fopen(const char *restrict filename, const char *restrict modes);

/**
 * opens a file with a given file name, replacing en existing stream with it
 *
 * @param filename - the name of the file to open
 * @param modes - the modes to open the file in
 * @param stream - the stream to replace
 * @returns the file pointer of success, NULL on error
 */
extern FILE *freopen(const char *restrict filename, const char *restrict modes,
					 FILE *restrict stream);

/**
 * closes a opened file
 *
 * @param stream - the opened file stream
 */
extern void fclose(FILE *stream);

/**
 * reads data from a file into a pointer
 *
 * @param ptr - the buffer to write into
 * @param size - the size of the block to read
 * @param n - the count of blocks to read
 * @param stream - the file stream to read from
 * @returns the number of blocks read
 */
extern size_t fread(void *restrict ptr, size_t size, size_t n,
					FILE *restrict stream);

/**
 * writes data from a pointer into a filename
 *
 * @param ptr - the buffer to read from
 * @param size - the size of the block to write
 * @param n - the count of blocks to write
 * @param stream - the file stream to write into
 * @returns the number of blocks written
 */
extern size_t fwrite(const void *restrict ptr, size_t size, size_t n,
					 FILE *restrict stream);

/**
 * seek to a certain position on stream
 *
 * @param stream - the stream to seek
 * @param off - the offset from whence
 * @param whence - where to seek from (SEEK_SET, SEEK_CUR, SEEK_END)
 * @returns 0 on success, -1 on error setting errno
 */
extern int fseek(FILE *stream, long int off, int whence);

/**
 * return the current position of stream
 *
 * @param stream - the stream to tell
 * @return the position on success, -1 on error setting errno
 */
extern long int ftell(FILE *stream);

/**
 * rewing to the begining of a stream
 *
 * @param stream - the stream to rewind
 */
extern void rewind(FILE *stream);

#endif /* stdio.h */
