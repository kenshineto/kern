#include <stdio.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
	FILE *in, *out;
	int length, value;

	if (argc != 3) {
		fprintf(stderr, "usage: inflate in out\n");
		return 1;
	}

	in = fopen(argv[1], "rb");
	if (in == NULL) {
		fprintf(stderr, "cannot read: %s\n", argv[1]);
		return 1;
	}

	out = fopen(argv[2], "wb");
	if (out == NULL) {
		fprintf(stderr, "cannot write %s\n", argv[2]);
		return 1;
	}

	while (1) {
		length = getc(in);
		value = getc(in);

		if (length == EOF)
			break;

		if (value == EOF)
			return 1;

		for (int i = 0; i < length; i++)
			putc(value, out);
	}

	return 0;
}
