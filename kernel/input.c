#include <comus/input.h>

struct mouse_event_buf {
	struct mouse_event data[N_MOUSEEV];
	size_t start;
	size_t end;
	size_t len;
};

struct keycode_buf {
	struct keycode data[N_KEYCODE];
	size_t start;
	size_t end;
	size_t len;
};

static struct keycode_buf keycode_buffer = {
	.start = 0,
	.end = 0,
	.len = 0,
};

static struct mouse_event_buf mouse_event_buffer = {
	.start = 0,
	.end = 0,
	.len = 0,
};

void keycode_push(struct keycode *ev)
{
	// save keycode
	keycode_buffer.data[keycode_buffer.end] = *ev;

	// update pointers
	keycode_buffer.end++;
	keycode_buffer.end %= N_KEYCODE;
	if (keycode_buffer.len < N_KEYCODE) {
		keycode_buffer.len++;
	} else {
		keycode_buffer.start++;
		keycode_buffer.start %= N_KEYCODE;
	}
}

int keycode_pop(struct keycode *ev)
{
	if (keycode_buffer.len < 1)
		return 1;

	*ev = keycode_buffer.data[keycode_buffer.start];
	keycode_buffer.len--;
	keycode_buffer.start++;
	keycode_buffer.start %= N_KEYCODE;
	return 0;
}

size_t keycode_len(void)
{
	return keycode_buffer.len;
}

void mouse_event_push(struct mouse_event *ev)
{
	// save mouse_event
	mouse_event_buffer.data[mouse_event_buffer.end] = *ev;

	// update pointers
	mouse_event_buffer.end++;
	mouse_event_buffer.end %= N_KEYCODE;
	if (mouse_event_buffer.len < N_KEYCODE) {
		mouse_event_buffer.len++;
	} else {
		mouse_event_buffer.start++;
		mouse_event_buffer.start %= N_KEYCODE;
	}
}

int mouse_event_pop(struct mouse_event *ev)
{
	if (mouse_event_buffer.len < 1)
		return 1;

	*ev = mouse_event_buffer.data[mouse_event_buffer.start];
	mouse_event_buffer.len--;
	mouse_event_buffer.start++;
	mouse_event_buffer.start %= N_KEYCODE;
	return 0;
}

size_t mouse_event_len(void)
{
	return mouse_event_buffer.len;
}
