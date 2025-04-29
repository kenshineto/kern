#include <comus/drivers/ps2.h>
#include <comus/keycodes.h>
#include <comus/asm.h>
#include <lib.h>

#define STATUS_OUT_BUF ((uint8_t)0x01)
#define STATUS_IN_BUF ((uint8_t)0x02)

#define CONFIG_INT_0 ((uint8_t)0x01)
#define CONFIG_INT_1 ((uint8_t)0x02)
#define CONFIG_SYS ((uint8_t)0x04)
#define CONFIG_CLOCK_0 ((uint8_t)0x10)
#define CONFIG_CLOCK_1 ((uint8_t)0x20)
#define CONFIG_TRANS ((uint8_t)0x40)

static uint8_t scancodes[] = {
	KEY_NONE,	KEY_F9,			 KEY_NONE,		KEY_F5,		   KEY_F3,
	KEY_F1,		KEY_F2,			 KEY_F12,		KEY_NONE,	   KEY_F10,
	KEY_F8,		KEY_F6,			 KEY_F4,		KEY_TAB,	   KEY_BACKTICK,
	KEY_NONE,	KEY_NONE,		 KEY_L_ALT,		KEY_L_SHIFT,   KEY_NONE,
	KEY_L_CTRL, KEY_Q,			 KEY_1,			KEY_NONE,	   KEY_NONE,
	KEY_NONE,	KEY_Z,			 KEY_S,			KEY_A,		   KEY_W,
	KEY_2,		KEY_NONE,		 KEY_NONE,		KEY_C,		   KEY_X,
	KEY_D,		KEY_E,			 KEY_4,			KEY_3,		   KEY_NONE,
	KEY_NONE,	KEY_SPACE,		 KEY_V,			KEY_F,		   KEY_T,
	KEY_R,		KEY_5,			 KEY_NONE,		KEY_NONE,	   KEY_N,
	KEY_B,		KEY_H,			 KEY_G,			KEY_Y,		   KEY_6,
	KEY_NONE,	KEY_NONE,		 KEY_NONE,		KEY_M,		   KEY_J,
	KEY_U,		KEY_7,			 KEY_8,			KEY_NONE,	   KEY_NONE,
	KEY_COMMA,	KEY_K,			 KEY_I,			KEY_O,		   KEY_0,
	KEY_9,		KEY_NONE,		 KEY_NONE,		KEY_PERIOD,	   KEY_SLASH,
	KEY_L,		KEY_SEMICOLON,	 KEY_P,			KEY_MINUS,	   KEY_NONE,
	KEY_NONE,	KEY_NONE,		 KEY_QUOTE,		KEY_NONE,	   KEY_L_BRACE,
	KEY_EQUAL,	KEY_NONE,		 KEY_NONE,		KEY_CAPS_LOCK, KEY_R_SHIFT,
	KEY_ENTER,	KEY_R_BRACE,	 KEY_NONE,		KEY_BACKSLASH, KEY_NONE,
	KEY_NONE,	KEY_NONE,		 KEY_NONE,		KEY_NONE,	   KEY_NONE,
	KEY_NONE,	KEY_NONE,		 KEY_BACKSPACE, KEY_NONE,	   KEY_NONE,
	KEY_NP_1,	KEY_NONE,		 KEY_NP_4,		KEY_NP_7,	   KEY_NONE,
	KEY_NONE,	KEY_NONE,		 KEY_NP_0,		KEY_NP_PERIOD, KEY_NP_2,
	KEY_NP_5,	KEY_NP_6,		 KEY_NP_8,		KEY_ESCAPE,	   KEY_NUM_LOCK,
	KEY_F11,	KEY_NP_PLUS,	 KEY_NP_3,		KEY_NP_MINUS,  KEY_NP_ASTERISK,
	KEY_NP_9,	KEY_SCROLL_LOCK, KEY_NONE,		KEY_NONE,	   KEY_NONE,
	KEY_NONE,	KEY_F7,
};
static uint8_t scancodes_ext[] = {
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_UNKNOWN, KEY_R_ALT,	  KEY_PRINT_SCREEN, KEY_NONE,
	KEY_R_CTRL,	  KEY_UNKNOWN, KEY_NONE,	  KEY_NONE,			KEY_UNKNOWN,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_L_META,  KEY_UNKNOWN,	  KEY_UNKNOWN,		KEY_NONE,
	KEY_UNKNOWN,  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_R_META,
	KEY_UNKNOWN,  KEY_NONE,	   KEY_NONE,	  KEY_UNKNOWN,		KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_MENU,	  KEY_UNKNOWN,		KEY_NONE,
	KEY_UNKNOWN,  KEY_NONE,	   KEY_UNKNOWN,	  KEY_NONE,			KEY_NONE,
	KEY_UNKNOWN,  KEY_UNKNOWN, KEY_NONE,	  KEY_UNKNOWN,		KEY_UNKNOWN,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_UNKNOWN,		KEY_UNKNOWN,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_UNKNOWN,	  KEY_NONE,			KEY_NP_SLASH,
	KEY_NONE,	  KEY_NONE,	   KEY_UNKNOWN,	  KEY_NONE,			KEY_NONE,
	KEY_UNKNOWN,  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NP_ENTER, KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_UNKNOWN,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_END,	  KEY_NONE,	   KEY_LEFT,	  KEY_HOME,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_INSERT,	  KEY_DELETE,		KEY_DOWN,
	KEY_NONE,	  KEY_RIGHT,   KEY_UP,		  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_PAGE_DOWN, KEY_NONE,			KEY_NONE,
	KEY_PAGE_UP,  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,	  KEY_NONE,	   KEY_NONE,	  KEY_NONE,			KEY_NONE,
	KEY_NONE,
};

// ctrl
static bool has_kbd = false;
static bool has_mouse = false;

// kbd
static struct keycode last_keycode = {
	.key = KEY_NONE,
	.flags = 0,
};
static bool state_keyup = false;
static bool state_ext = false;

// mouse
static struct mouse_event last_mouse_ev = {
	.updated = false,
	.lmb = false,
	.mmb = false,
	.rmb = false,
	.relx = 0,
	.rely = 0,
};
static uint8_t first_b, second_b, third_b;

static uint8_t ps2ctrl_in_status(void)
{
	return inb(0x64);
}

static uint8_t ps2ctrl_in(void)
{
	while ((ps2ctrl_in_status() & STATUS_OUT_BUF) == 0) {
		io_wait();
	}
	return inb(0x60);
}

static void ps2ctrl_out_cmd(uint8_t cmd)
{
	while ((ps2ctrl_in_status() & STATUS_IN_BUF) != 0) {
		io_wait();
	}
	outb(0x64, cmd);
}

static void ps2ctrl_out_data(uint8_t data)
{
	while ((ps2ctrl_in_status() & STATUS_IN_BUF) != 0) {
		io_wait();
	}
	outb(0x60, data);
}

static void ps2ctrl_set_port2(void)
{
	outb(0x64, 0xD4);
}

static int ps2kb_init(void)
{
	uint8_t result;

	ps2ctrl_out_data(0xFF);
	if ((result = ps2ctrl_in()) != 0xFA)
		return 1;

	if ((result = ps2ctrl_in()) != 0xAA)
		return 1;

	ps2ctrl_out_data(0xF4);
	if ((result = ps2ctrl_in()) != 0xFA)
		return 1;

	has_kbd = true;
	return 0;
}

static int ps2mouse_init(void)
{
	uint8_t result;

	ps2ctrl_set_port2();
	ps2ctrl_out_data(0xFF);
	if ((result = ps2ctrl_in()) != 0xFA)
		return 1;

	if ((result = ps2ctrl_in()) != 0xAA)
		return 1;

	ps2ctrl_set_port2();
	ps2ctrl_out_data(0xF4);

	has_mouse = true;
	return 0;
}

void ps2kb_recv(void)
{
	uint8_t code;

	if (!has_kbd)
		return;

	code = ps2ctrl_in();
	if (code == 0x00 || code == 0x0F) {
		last_keycode.key = KEY_NONE;
		last_keycode.flags = KC_FLAG_ERROR;
	} else if (code == 0xF0) {
		state_keyup = true;
	} else if (code == 0xE0) {
		state_ext = true;
	} else if (code <= 0x84) {
		uint8_t *scancode_table = state_ext ? scancodes_ext : scancodes;
		uint8_t keycode = scancode_table[code];
		if (keycode != KEY_NONE) {
			last_keycode.key = keycode;
			last_keycode.flags = state_keyup ? KC_FLAG_KEY_UP :
											   KC_FLAG_KEY_DOWN;
		}
		state_keyup = false;
		state_ext = false;
	}
}

struct keycode ps2kb_get(void)
{
	struct keycode code;

	if (!has_kbd)
		return last_keycode;

	code = last_keycode;
	last_keycode.key = KEY_NONE;
	last_keycode.flags = 0;
	return code;
}

void ps2mouse_recv(void)
{
	static uint8_t packet_num = 0;
	uint8_t code;

	if (!has_mouse)
		return;

	code = ps2ctrl_in();
	switch (packet_num) {
	case 0:
		first_b = code;
		break;
	case 1:
		second_b = code;
		break;
	case 2: {
		int state, d;

		third_b = code;
		state = first_b;
		d = second_b;
		last_mouse_ev.relx = d - ((state << 4) & 0x100);
		d = third_b;
		last_mouse_ev.rely = d - ((state << 3) & 0x100);

		last_mouse_ev.lmb = first_b & 0x01;
		last_mouse_ev.rmb = first_b & 0x02;
		last_mouse_ev.mmb = first_b & 0x04;
		last_mouse_ev.updated = true;
		break;
	}
	}

	packet_num += 1;
	packet_num %= 3;
}

struct mouse_event ps2mouse_get(void)
{
	struct mouse_event event = last_mouse_ev;
	last_mouse_ev.updated = false;
	return event;
}

int ps2_init(void)
{
	uint8_t result;

	cli();

	inb(0x60);

	// self-test
	ps2ctrl_out_cmd(0xAA);
	if ((result = ps2ctrl_in()) != 0x55) {
		WARN("PS/2 Controller failed to initalize.");
		return 1;
	}

	// set config
	ps2ctrl_out_cmd(0x20);
	uint8_t config = ps2ctrl_in();
	config = (config | CONFIG_INT_0 | CONFIG_INT_1) & ~CONFIG_TRANS;
	// config = 0xFF;
	ps2ctrl_out_cmd(0x60);
	ps2ctrl_out_data(config);

	// enable port 0
	ps2ctrl_out_cmd(0xAE);

	// enable port 2
	ps2ctrl_out_cmd(0xA9);
	if ((result != ps2ctrl_in()) != 0x01) {
		WARN("PS/2 port 2 not supported");
		return 1;
	}

	ps2ctrl_out_cmd(0xA8);

	ps2kb_init();
	ps2mouse_init();
	sti();
	return 0;
}
