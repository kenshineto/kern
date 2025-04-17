/**
 * @file kctype.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Kernel C type libaray functions
 */

#ifndef _KCTYPE_H
#define _KCTYPE_H

/**
 * @returns 1 if c is a space
 */
int isspace(int c);

/**
 * @returns 1 if c is a digit (0 - 9)
 */
int isdigit(int c);

/**
 * @returns if a character is ascii printable
 */
int isprint(int c);

#endif /* kctype.h */
