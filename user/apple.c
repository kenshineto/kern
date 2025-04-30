#include <incbin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

INCBIN(APPLE, "data/apple.bin");

#define APPLE_WIDTH 480
#define APPLE_HEIGHT 360
#define APPLE_FPS 30
#define APPLE_FRAMES 6572
#define VSYNC_FPS 60

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// returned from drm
static uint32_t *fb;
static int width, height, bpp, scale;
size_t fb_size;

// decompress state
static size_t d_offset = 0, d_length = 0;
static unsigned char d_value;

// frame counter
static size_t last_frame = SIZE_MAX, frame = 0;

// frame buffer
static unsigned char frame_buf[APPLE_WIDTH * APPLE_HEIGHT];

// backbuffer
static uint32_t *back_fb;

// start tick
static size_t tick_start;

static unsigned char read_byte(void)
{
	while (d_length < 1) {
		unsigned char len;

		d_length = 0;
		do {
			len = gAPPLEData[d_offset++];
			d_length += len;
		} while (len == 255);
		d_value = gAPPLEData[d_offset++];
	}

	d_length--;
	return d_value;
}

static void read_frame(void)
{
	for (size_t i = 0; i < sizeof(frame_buf); i++)
		frame_buf[i] = read_byte();
}

static void swap_buffers(void)
{
	// terrible :3
	uint32_t count = width * height / 2;
	__asm__ volatile("rep movsq" ::"S"(back_fb), "D"(fb), "c"(count)
					 : "memory");
}

static void draw_frame(void)
{
	for (int y = 0; y < APPLE_HEIGHT * scale; y++) {
		for (int x = 0; x < APPLE_WIDTH * scale; x++) {
			unsigned char color =
				frame_buf[(x / scale) + (y / scale) * APPLE_WIDTH];
			back_fb[x + y * width] = (color << 16) | (color << 8) |
									 (color << 0);
		}
	}
}

int main(void)
{
	printf("all your apple belong to bad\n");

	if (drm((void **)&fb, &width, &height, &bpp)) {
		fprintf(stderr, "failure!\n");
		return 1;
	}

	fb_size = (width * height * bpp) / 8;
	back_fb = malloc(fb_size);
	if (back_fb == NULL) {
		fprintf(stderr, "failure but 2\n");
		return 1;
	}

	tick_start = ticks();
	scale = MIN(width / APPLE_WIDTH, height / APPLE_HEIGHT);

	while (1) {
		if (last_frame != frame) {
			read_frame();
			draw_frame();
			swap_buffers();
		}

		last_frame = frame;
		frame = ((ticks() - tick_start) / (1000 / APPLE_FPS));
		if (frame >= APPLE_FRAMES)
			break;
	}

	return 0;
}
