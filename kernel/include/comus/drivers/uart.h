/**
 * @file uart.h
 *
 * @author Freya Murphy <freya@freyacat.org>
 *
 * Serial interface
 */

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

int uart_init(void);
uint8_t uart_in(void);
void uart_out(uint8_t ch);
void uart_out_str(const char *str);

#endif /* uart.h */
