/*
 * FORKMAN: a platformer game with IPC
 *
 * This executable will fork. One process is responsible for writing to the
 * screen and getting user input, the other is responsible for game logic.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
	volatile uint8_t mem[PAGE_SIZE * (SHARED_PAGES - 1)];
} sharedmem;

static int display_server_entry(sharedmem *);
static int client_entry(sharedmem *);

int main(void)
{
	uint8_t *current_break = sbrk(0);
	if (!current_break) {
		fprintf(stderr, "sbrk failure\n");
		return 1;
	}

	printf("ABOUT TO FORK\n");

	int child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return 1;
	}

	if (child) {
		printf("FORK GOOD, CHILD IS %d\n", child);
		sharedmem *shared = allocshared(SHARED_PAGES, child);
		if (!shared) {
			fprintf(stderr, "memory share failure\n");
			return 1;
		}

		return display_server_entry(shared);
	} else {
		printf("FORK GOOD, WE ARE CHILD\n");

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

	while (1) {
		if (shared->frame == last_frame) {
			continue;
		}

		printf("server writing frame %zu\n", last_frame);

		last_frame += 1;
		shared->frame = last_frame;
	}

	return 0;
}

static int client_entry(sharedmem *shared)
{
	size_t last_frame = shared->frame;
	do {
		if (last_frame == shared->frame)
			continue;

		printf("client writing frame %zu\n", last_frame);

		last_frame += 1;
		shared->frame = last_frame;
	} while (1);

	return 0;
}
