
#include <incbin.h>
#include <stdio.h>
#include <unistd.h>

INCBIN(APPLE, "data/apple.bin");

#define APPLE_WIDTH 256
#define APPLE_HEIGHT 144
#define APPLE_FPS 12
#define APPLE_FRAMES 2630

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// returned from drm
static uint32_t *fb;
static int width, height, bpp, scale;

// meta
static size_t frame = 0;
static size_t frame_size;
static size_t ticks_off;

#define PIXEL(x, y)                                         \
	gAPPLEData[offset +                                     \
			   (((x + APPLE_WIDTH % APPLE_WIDTH) / scale) + \
				((y + APPLE_HEIGHT % APPLE_HEIGHT) / scale) * APPLE_WIDTH)]

static void draw_frame(void)
{
	size_t offset = frame_size * frame;

	for (int y = 0; y < APPLE_HEIGHT * scale; y++) {
		for (int x = 0; x < APPLE_WIDTH * scale; x++) {
			uint8_t colors[9];
			colors[0] = PIXEL(x, y);
			colors[1] = PIXEL(x - 1, y);
			colors[2] = PIXEL(x + 1, y);
			colors[3] = PIXEL(x, y - 1);
			colors[4] = PIXEL(x - 1, y - 1);
			colors[5] = PIXEL(x + 1, y - 1);
			colors[6] = PIXEL(x, y + 1);
			colors[7] = PIXEL(x - 1, y + 1);
			colors[8] = PIXEL(x + 1, y + 1);

			// anti aliasing
			uint8_t color = (colors[0] + colors[0] + colors[0] + colors[0] +
							 colors[1] + colors[2] + colors[3] + colors[4] +
							 colors[5] + colors[6] + colors[7] + colors[8]) /
							12;

			fb[x + y * width] = (color << 16) | (color << 8) | (color << 0);
		}
	}

	frame = ((ticks() - ticks_off) / (1000 / APPLE_FPS));

	if (frame >= APPLE_FRAMES)
		exit(0);
}

int main(void)
{
	printf("all your apple belong to bad\n");

	if (drm((void **)&fb, &width, &height, &bpp)) {
		fprintf(stderr, "failure!\n");
		return 1;
	}

	ticks_off = ticks();
	frame_size = APPLE_WIDTH * APPLE_HEIGHT;
	scale = MIN(width / APPLE_WIDTH, height / APPLE_HEIGHT);

	while (1)
		draw_frame();

	return 0;
}
