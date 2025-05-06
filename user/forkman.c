/*
 * FORKMAN: a platformer game with IPC
 *
 * This executable will fork. One process is responsible for writing to the
 * screen and getting user input, the other is responsible for game logic.
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include "../kernel/include/comus/keycodes.h"

#define DBG

#define GAME_WIDTH 480
#define GAME_HEIGHT 360
#define SHARED_PAGES 10
#define PAGE_SIZE 4096
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define GAME_WIDTH_TILES (GAME_WIDTH / TILE_WIDTH)
#define GAME_HEIGHT_TILES (GAME_HEIGHT / TILE_HEIGHT)
#define PLAYER_WIDTH 10
#define PLAYER_HEIGHT 10

typedef struct {
	double x;
	double y;
} vec;

typedef struct {
	uint32_t *mapped_memory;
	int width;
	int height;
	int bpp;
	int scale;
	size_t size;
} framebuffer;

enum tile_type {
	TILE_AIR,
	TILE_SOLID,
	TILE_GRATE,
};

typedef struct {
	enum tile_type type;
} tile;

#define CLIENT_BIT (1)
#define SERVER_BIT (2)
#define CLIENT_WAITING_BIT (1)
#define SERVER_WAITING_BIT (2)

typedef struct {
	volatile uint8_t inner;
} spinlock;

enum key_state {
	KEY_STATE_PRESSED,
	KEY_STATE_UNPRESSED,
	// TODO: JUST_PRESSED and JUST_RELEASED
};

typedef struct {
	volatile size_t frame;
	volatile size_t dummy_counter;
	volatile uint8_t client_barrier;
	volatile uint8_t server_barrier;
	volatile enum key_state key_status[255];
	spinlock player_lock;
	volatile vec player_pos;
	volatile vec player_vel;
	volatile tile tiles[GAME_HEIGHT_TILES * GAME_WIDTH_TILES];
	volatile uint8_t mem[PAGE_SIZE * (SHARED_PAGES - 1)];
} sharedmem;

static int display_server_entry(sharedmem *);
static int client_entry(sharedmem *);
static void barrier_wait(sharedmem *, int isclient);
static volatile tile *tile_at(sharedmem *, size_t x, size_t y);
static int is_inbounds(size_t x, size_t y);
static void spinlock_lock(volatile spinlock *lock, int isclient);
static void spinlock_unlock(volatile spinlock *lock, int isclient);

int main(void)
{
	// static_assert where are you :(
	if (SHARED_PAGES * 4096 < sizeof(sharedmem)) {
		fprintf(stderr, "bad memory configuration");
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
	const size_t idx = x + y * fb->width;
#ifdef DBG
	if (idx > fb->size) {
		printf("overflow?\n");
		exit(0);
	}
#endif
	fb->mapped_memory[idx] = state * (uint32_t)-1;
}

static void draw_filled(framebuffer *fb, const size_t x, const size_t y,
						int state)
{
	for (size_t rx = x * TILE_WIDTH; rx < (x + 1) * TILE_WIDTH; ++rx) {
		for (size_t ry = y * TILE_HEIGHT; ry < (y + 1) * TILE_HEIGHT; ++ry) {
			set_pixel(fb, rx, ry, state);
		}
	}
}

static void draw_grate(framebuffer *fb, const size_t x, const size_t y)
{
	for (size_t rx = x * TILE_WIDTH; rx < (x + 1) * TILE_WIDTH; ++rx) {
		for (size_t ry = y * TILE_HEIGHT; ry < (y + 1) * TILE_HEIGHT; ++ry) {
			int state;
			if (x == y) {
				state = 1;
			} else {
				state = 0;
			}
			set_pixel(fb, rx, ry, state);
		}
	}
}

static void draw_tiles(sharedmem *shared, framebuffer *fb)
{
	for (size_t x = 0; x < GAME_WIDTH_TILES; ++x) {
		for (size_t y = 0; y < GAME_HEIGHT_TILES; ++y) {
			volatile tile *tile = tile_at(shared, x, y);
			switch (tile->type) {
			case TILE_AIR:
				draw_filled(fb, x, y, 0);
				break;
			case TILE_SOLID:
				draw_filled(fb, x, y, 1);
				break;
			case TILE_GRATE:
				draw_grate(fb, x, y);
				break;
			}
		}
	}
}

static void draw_player(framebuffer *fb, vec pos)
{
	for (size_t x = pos.x; x < ((size_t)pos.x + PLAYER_WIDTH); ++x) {
		for (size_t y = pos.y; y < ((size_t)pos.y + PLAYER_WIDTH); ++y) {
			set_pixel(fb, x, y, 1);
		}
	}
}

static void init_level(sharedmem *shared)
{
	for (size_t i = 0; i < GAME_WIDTH_TILES; ++i) {
		tile_at(shared, i, 10)->type = TILE_GRATE;
	}

	shared->player_pos = (vec){ .x = 5 * TILE_WIDTH, .y = 5 * TILE_HEIGHT };
}

static size_t get_total_time(size_t tick_start) // arbitrary units
{
	// 60 is arbitrary pretend fps
	return ((ticks() - tick_start) / (1000 / 60));
}

static int display_server_entry(sharedmem *shared)
{
	framebuffer fb;
	if (drm((void **)&fb.mapped_memory, &fb.width, &fb.height, &fb.bpp)) {
		fprintf(stderr, "Unable to map framebuffer, display server failing\n");
		return 1;
	}

	fb.size = (fb.width * fb.height * fb.bpp) / 8;

	barrier_wait(shared, 0);

	while (1) {
		struct keycode keycode;

		if (keypoll(&keycode)) {
			if (keycode.flags & KC_FLAG_KEY_DOWN) {
				shared->key_status[(uint8_t)keycode.key] = KEY_STATE_PRESSED;
			}
			if (keycode.flags & KC_FLAG_KEY_UP) {
				shared->key_status[(uint8_t)keycode.key] = KEY_STATE_UNPRESSED;
			}
		}

		draw_tiles(shared, &fb);

		spinlock_lock(&shared->player_lock, 0);
		draw_player(&fb, shared->player_pos);
		spinlock_unlock(&shared->player_lock, 0);
	}

	return 0;
}

static int client_entry(sharedmem *shared)
{
	init_level(shared);

	size_t start_ticks = ticks();

	double last_time = get_total_time(start_ticks);

	barrier_wait(shared, 1);
	do {
		double time = get_total_time(start_ticks);
		double delta_time = time - last_time;
		spinlock_lock(&shared->player_lock, 1);

		shared->player_vel.y -= 9.8 * delta_time;

		// framerate dependent...
		const vec drag = {
			.x = shared->player_vel.x * 0.1,
			.y = shared->player_vel.y * 0.1,
		};
		shared->player_vel.x -= drag.x;
		shared->player_vel.y -= drag.y;

		for (size_t i = 0; i < shared->player_vel.x; ++i) {
			size_t x = shared->player_pos.x + 1;
			size_t y = shared->player_pos.y + 1;
			// boundscheck to consider outside tiles to be solid
			if (is_inbounds(x, y) && tile_at(shared, x, y)->type != TILE_AIR) {
				break;
			}
			shared->player_pos.x += 1;
			shared->player_pos.y += 1;
		}

		if (shared->key_status[KEY_SPACE] == KEY_STATE_PRESSED) {
			shared->player_vel.y = 10;
		} else if (shared->key_status[KEY_B] == KEY_STATE_PRESSED) {
			shared->player_vel.y = -10;
		}
		spinlock_unlock(&shared->player_lock, 1);

	} while (1);

	return 0;
}

static volatile tile *tile_at(sharedmem *shared, size_t x, size_t y)
{
#ifdef DBG
	if (!is_inbounds(x, y)) {
		printf("out of bounds");
		exit(0);
	}
#endif
	const size_t idx = x + (y * GAME_WIDTH_TILES);
	return shared->tiles + idx;
}

static int is_inbounds(size_t x, size_t y)
{
	const size_t idx = x + (y * GAME_WIDTH_TILES);
	return idx < (GAME_WIDTH_TILES * GAME_HEIGHT_TILES);
}

static void spinlock_lock(volatile spinlock *lock, int isclient)
{
	const uint8_t bit = isclient ? CLIENT_BIT : SERVER_BIT;
	const uint8_t otherbit = isclient ? SERVER_BIT : CLIENT_BIT;

	// wait for us to be the only waiter
	while (1) {
		if (lock->inner == 0) {
			lock->inner |= bit;
			// recover from the possibility that something happened between that
			// if statement and the |= operation. check for other bits being set
			if ((lock->inner ^ bit) != 0) {
				// okay, somebody messed with something, undo what we did and
				// keep waiting
				lock->inner ^= bit;
				continue;
			}

			// okay all good, we are the exclusive owner
			break;
		}
	}

#ifdef DBG
	// when we own the lock, only our bit should be active and the other proc's
	// bit should not be active
	if (lock->inner & otherbit || !(lock->inner & bit)) {
		printf("SPINLOCK BAD\n");
		exit(1);
	}
#endif
}

static void spinlock_unlock(volatile spinlock *lock, int isclient)
{
	const uint8_t bit = isclient ? CLIENT_BIT : SERVER_BIT;

	// assume our thread of execution (process) is the only one changing our
	// bit. this can be screwed over if the other process lies about whether
	// it is server or client
	lock->inner ^= bit;
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
