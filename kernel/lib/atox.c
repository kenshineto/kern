#include <lib.h>

#define ATOX(name, type)             \
	type name(const char *s)         \
	{                                \
		for (; isspace(*s); s++)     \
			;                        \
		int neg = 0;                 \
		switch (*s) {                \
		case '-':                    \
			neg = 1;                 \
			/* fallthrough */        \
		case '+':                    \
			s++;                     \
			break;                   \
		}                            \
		type num = 0;                \
		for (; *s == '0'; s++)       \
			;                        \
		for (; isdigit(*s); s++) {   \
			num *= 10;               \
			num += *s - '0';         \
		}                            \
		return num * (neg ? -1 : 1); \
	}

ATOX(atoi, int)
ATOX(atol, long int)
ATOX(atoll, long long int)
