#include <stdlib.h>
#include <ctype.h>

#define STRTOX(name, type)                                               \
	type name(const char *restrict s, char **restrict endptr, int radix) \
	{                                                                    \
		const char *s_start = s;                                         \
		for (; isspace(*s); s++)                                         \
			;                                                            \
                                                                         \
		int sign = 0;                                                    \
		switch (*s) {                                                    \
		case '-':                                                        \
			sign = 1; /* fallthrough */                                  \
		case '+':                                                        \
			s++;                                                         \
			break;                                                       \
		}                                                                \
                                                                         \
		if ((radix == 0 || radix == 16) &&                               \
			(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))) {             \
			radix = 16;                                                  \
			s += 2;                                                      \
		} else if (radix == 0) {                                         \
			if (*s == '0') {                                             \
				radix = 8;                                               \
				s++;                                                     \
			} else {                                                     \
				radix = 10;                                              \
			}                                                            \
		}                                                                \
                                                                         \
		type num = 0;                                                    \
		int has_digit = 0;                                               \
                                                                         \
		while (1) {                                                      \
			int n = ctoi(*s++);                                          \
			if (n < 0 || n >= radix)                                     \
				break;                                                   \
			has_digit = 1;                                               \
			num = num * radix + n;                                       \
		}                                                                \
                                                                         \
		if (endptr != NULL) {                                            \
			*endptr = has_digit ? (char *)(s - 1) : (char *)s_start;     \
		}                                                                \
                                                                         \
		return sign ? -num : num;                                        \
	}

STRTOX(strtoi, int)
STRTOX(strtol, long int)
STRTOX(strtoll, long long int)
