#include <stdlib.h>
#include <ctype.h>

#define STRTOUX(name, type)                                              \
	type name(const char *restrict s, char **restrict endptr, int radix) \
	{                                                                    \
		const char *s_start = s;                                         \
		for (; isspace(*s); s++)                                         \
			;                                                            \
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
		return num;                                                      \
	}

STRTOUX(strtoui, unsigned int)
STRTOUX(strtoul, unsigned long int)
STRTOUX(strtoull, unsigned long long int)
