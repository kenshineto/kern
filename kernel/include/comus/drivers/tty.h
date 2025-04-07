/**
 * @file tty.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Terminal output
 */

#ifndef TTY_H_
#define TTY_H_

/**
 * Output a character to the terminal
 */
void tty_out(char c);

/**
 * Output a string to the terminal
 */
void tty_out_str(char *str);

#endif /* tty.h */
