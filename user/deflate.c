#include <stdio.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
	FILE *in, *out;
	int length_bytes = 0, length = 0, tracking = EOF, next;

	if (argc != 3) {
		fprintf(stderr, "usage: deflate in out\n");
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

	while (next = getc(in), next != EOF) {
		int diff;

		if (tracking == EOF) {
			tracking = next;
			length = 1;
			length_bytes = 0;
			continue;
		}

		diff = tracking - next;
		diff = diff < 1 ? -diff : diff;
		if (diff < 2)
			next = tracking;

		if (tracking == next) {
			length++;
			if (length == 255) {
				length_bytes++;
				length = 0;
			}
			continue;
		}

		// write output
		for (int i = 0; i < length_bytes; i++)
			putc(255, out);
		putc(length, out);
		putc(tracking, out);
		tracking = next;
		length = 1;
		length_bytes = 0;
	}

	if (length) {
		for (int i = 0; i < length_bytes; i++)
			putc(255, out);
		putc(length, out);
		putc(tracking, out);
	}

	return 0;
}
