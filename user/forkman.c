/*
 * FORKMAN: a platformer game with IPC
 *
 * This executable will fork. One process is responsible for writing to the
 * screen and getting user input, the other is responsible for game logic.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include "../kernel/include/comus/keycodes.h"

#define GAME_WIDTH 480
#define GAME_HEIGHT 360
#define SHARED_PAGES 10
#define PAGE_SIZE 4096

typedef struct {
	uint32_t *mapped_memory;
	int width;
	int height;
	int bpp;
	int scale;
	size_t size;
} framebuffer;

typedef struct {
	volatile size_t frame;
	volatile size_t dummy_counter;
	volatile int lock;
	volatile uint8_t client_barrier;
	volatile uint8_t server_barrier;
	volatile uint8_t key_status[255];
	volatile uint8_t mem[PAGE_SIZE * (SHARED_PAGES - 1)];
} sharedmem;

static int display_server_entry(sharedmem *);
static int client_entry(sharedmem *);
static void barrier_wait(sharedmem *, int isclient);

int main(void)
{
	uint8_t *current_break = sbrk(0);
	if (!current_break) {
		fprintf(stderr, "sbrk failure\n");
		return 1;
	}

	int child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return 1;
	}

	if (child) {
		sharedmem *shared = allocshared(SHARED_PAGES, child);
		if (!shared) {
			fprintf(stderr, "memory share failure\n");
			return 1;
		}

		return display_server_entry(shared);
	} else {
		sharedmem *shared;

		while (!(shared = popsharedmem()))
			sleep(1);

		return client_entry(shared);
	}
}

static void set_pixel(framebuffer *fb, size_t x, size_t y,
					  int state) // state is 0 or 1
{
	fb->mapped_memory[x + y * fb->width] = state * (0x00010101);
}

static int display_server_entry(sharedmem *shared)
{
	framebuffer fb;
	if (drm((void **)&fb.mapped_memory, &fb.width, &fb.height, &fb.bpp)) {
		fprintf(stderr, "Unable to map framebuffer, display server failing\n");
		return 1;
	}

	fb.size = (fb.width * fb.height * fb.bpp) / 8;

	size_t last_frame = 1;

	barrier_wait(shared, 0);

	while (1) {
		if (shared->frame == last_frame)
			continue;

		struct keycode keycode;

		if (keypoll(&keycode)) {
			if (keycode.flags & KC_FLAG_KEY_DOWN) {
				shared->key_status[(uint8_t)keycode.key] = 1;
			}
			if (keycode.flags & KC_FLAG_KEY_UP) {
				shared->key_status[(uint8_t)keycode.key] = 0;
			}
		}

		last_frame += 1;
		shared->frame = last_frame;
	}

	return 0;
}

static int client_entry(sharedmem *shared)
{
	size_t last_frame = shared->frame;
	barrier_wait(shared, 1);
	do {
		if (last_frame == shared->frame)
			continue;

		last_frame += 1;
		shared->frame = last_frame;
	} while (1);

	return 0;
}

static void barrier_wait(sharedmem *shared, int isclient)
{
	if (isclient) {
		if (shared->server_barrier) {
			shared->server_barrier = 0;
		} else {
			shared->client_barrier = 1;
			while (shared->client_barrier)
				;
		}
	} else {
		if (shared->client_barrier) {
			shared->client_barrier = 0;
		} else {
			shared->server_barrier = 1;
			while (shared->server_barrier)
				;
		}
	}
}
