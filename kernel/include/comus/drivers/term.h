/**
 * @file tty.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Terminal output
 */

#ifndef TERM_H_
#define TERM_H_

/**
 * Output a character to the terminal
 */
void term_out(char c);

/**
 * Output a string to the terminal
 */
void term_out_str(const char *str);

/**
 * Clear all lines on the terminal
 */
void term_clear(void);

/**
 * Clear a specifc line on the terminal terminal
 */
void term_clear_line(int line);

/**
 * Scroll terminal
 */
void term_scroll(int lines);

#endif /* tty.h */
