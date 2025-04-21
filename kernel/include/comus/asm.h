#pragma once

#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t ret;
	__asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void outw(uint16_t port, uint16_t val)
{
	__asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t ret;
	__asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void insl(uint16_t port, uint32_t *buffer, uint32_t count)
{
	while (count--) {
		__asm__ volatile("inl %1, %0" : "=a"(*buffer) : "Nd"(port));
		buffer++;
	}
}

static inline void outl(uint16_t port, uint32_t val)
{
	__asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
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
