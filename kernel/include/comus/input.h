/**
 * @file input.h
 */

#ifndef INPUT_H_
#define INPUT_H_

#include <comus/keycodes.h>
#include <comus/limits.h>
#include <stdbool.h>
#include <stddef.h>

struct keycode {
	char key;
	char flags;
};

struct mouse_event {
	bool updated;
	bool lmb;
	bool rmb;
	bool mmb;
	int relx;
	int rely;
};

void keycode_push(struct keycode *ev);
int keycode_pop(struct keycode *ev);
size_t keycode_len(void);
char keycode_to_char(struct keycode *ev);

void mouse_event_push(struct mouse_event *ev);
int mouse_event_pop(struct mouse_event *ev);
size_t mouse_event_len(void);

#endif /* input.h */
