/**
 * @file term.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Terminal output
 */

#ifndef TERM_H_
#define TERM_H_

#include <stdint.h>

/**
 * Get the character width of the terminal
 */
uint16_t term_width(void);

/**
 * Get the character height of the terminal
 */
uint16_t term_height(void);

/**
 * Output a character to the terminal
 */
void term_out(char c);

/**
 * Output a character to the terminal at a given position
 */
void term_out_at(char c, uint16_t x, uint16_t y);

/**
 * Output a string to the terminal
 */
void term_out_str(const char *str);

/**
 * Output a string to the terminal at a given position
 */
void term_out_str_at(const char *str, uint16_t x, uint16_t y);

/**
 * Clear all lines on the terminal
 */
void term_clear(void);

/**
 * Clear a specifc line on the terminal terminal
 */
void term_clear_line(uint16_t line);

/**
 * Redraw terminal
 */
void term_redraw(void);

/**
 * Scroll terminal
 */
void term_scroll(uint16_t lines);

/**
 * Resize terminal
 */
void term_resize(uint16_t width, uint16_t height);

#endif /* term.h */
