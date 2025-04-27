#pragma once

#include <stdint.h>
#include <stddef.h>

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void rep_inb(uint16_t port, uint8_t *buffer, size_t count)
{
	while (count--)
		*(buffer++) = inb(port);
}

static inline void outb(uint16_t port, uint8_t val)
{
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void rep_outb(uint16_t port, uint8_t *buffer, size_t count)
{
	while (count--)
		outb(port, *(buffer++));
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t ret;
	__asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void rep_inw(uint16_t port, uint16_t *buffer, size_t count)
{
	__asm__ volatile("rep insw"
					 : "+D"(buffer), "+c"(count)
					 : "d"(port)
					 : "memory");
}

static inline void outw(uint16_t port, uint16_t val)
{
	__asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void rep_outw(uint16_t port, uint16_t *buffer, size_t count)
{
	while (count--)
		outw(port, *(buffer++));
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void rep_inl(uint16_t port, uint32_t *buffer, size_t count)
{
	while (count--)
		*(buffer++) = inl(port);
}

static inline void outl(uint16_t port, uint32_t val)
{
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline void rep_outl(uint16_t port, uint32_t *buffer, size_t count)
{
	while (count--)
		outl(port, *(buffer++));
}

static inline void io_wait(void)
{
	outb(0x80, 0);
}

static inline void sti(void)
{
	__asm__ volatile("sti");
}

static inline void cli(void)
{
	__asm__ volatile("cli");
}

static inline void int_wait(void)
{
	__asm__ volatile("sti; hlt");
}

static inline void halt(void)
{
	__asm__ volatile("cli; hlt");
}
