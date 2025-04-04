#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define PRINTF_NUMERIC_BUF_LEN 50

typedef union {
	unsigned long long int u;
	signed long long int i;
	char *str;
	char c;
} data_t;

/// options that can be set inside a specifier
/// flags, width, precision, length, and data type
typedef struct {
	/* flags */
	/// left justify content
	uint8_t left : 1;
	/// force sign (+/-) on numeric output
	uint8_t sign : 1;
	/// leave space if no printed sign on numeric output
	uint8_t space : 1;
	/// preceed hex/octal output with '0x'
	uint8_t hash : 1;
	/// left pads numeric output with zeros
	uint8_t zero : 1;
	uint8_t : 3;

	/* width & precision */
	/// minimum number of characters to be printed (padding if origonal is less)
	int width;
	/// digit precision used when printing numerical answers
	int precision;
	/// if a fixed minimum width has been provided
	uint8_t width_set : 1;
	/// if the provided minimum width is in the next variable argument
	uint8_t width_varies : 1;
	/// if a fixed digit precision has been provided
	uint8_t precision_set : 1;
	/// if the provided digit precision is in the next variable argument
	uint8_t precision_varies : 1;
	uint8_t : 4;

	/* length */
	/// what size to read argument as
	enum printf_len {
		PRINTF_LEN_CHAR,
		PRINTF_LEN_SHORT_INT,
		PRINTF_LEN_INT,
		PRINTF_LEN_LONG_INT,
		PRINTF_LEN_LONG_LONG_INT,
		PRINTF_LEN_SIZE_T,
	} len;

	/* other */
	/// radix to print the numerical answers as
	uint8_t radix;
	/// case to print hexadecimal values as
	bool is_uppercase;
} options_t;

typedef struct {
	/* input */
	/// the origonal format string
	const char *format;
	/// maximum allowed output length
	size_t max_len;
	/// if a maximum output length is set
	bool has_max_len;

	/* output */
	size_t written_len;
	bool to_file;
	union {
		FILE *file;
		char *buf;
	} out;

	/* pass 2 */
	char *output;
} context_t;

static void printf_putc(context_t *ctx, char c)
{
	// bounds check
	if (ctx->has_max_len)
		if (ctx->written_len >= ctx->max_len)
			return;

	// write to correct
	if (ctx->to_file)
		fputc(ctx->out.file, c);
	else
		*(ctx->out.buf++) = c;

	ctx->written_len++;
}

static int parse_flag(const char **res, options_t *opts)
{
	const char *fmt = *res;
	switch (*(fmt++)) {
	case '-':
		opts->left = 1;
		break;
	case '+':
		opts->sign = 1;
		break;
	case ' ':
		opts->space = 1;
		break;
	case '#':
		opts->hash = 1;
		break;
	case '0':
		opts->zero = 1;
		break;
	default:
		return 0;
	}

	*res = fmt;
	return 1;
}

static void parse_width(const char **res, options_t *opts)
{
	const char *fmt = *res;
	char *end = NULL;

	// check varies
	if (*fmt == '*') {
		opts->width_varies = true;
		*res = fmt++;
		return;
	}

	// parse num
	long width = strtol(fmt, &end, 10);
	if (end != NULL) {
		opts->width_set = 1;
		opts->width = width;
		*res = end;
		return;
	}
}

static void parse_precision(const char **res, options_t *opts)
{
	const char *fmt = *res;
	char *end = NULL;

	// check for dot
	if (*(fmt++) != '.')
		return;

	// check varies
	if (*fmt == '*') {
		opts->precision_varies = true;
		*res = fmt++;
		return;
	}

	// parse num
	long precision = strtol(fmt, &end, 10);
	if (end != NULL) {
		opts->precision_set = 1;
		opts->precision = precision;
		*res = end;
		return;
	}
}

static void parse_length(const char **res, options_t *opts)
{
	const char *fmt = *res;

	switch (*(fmt++)) {
	// half
	case 'h':
		if (*fmt == 'h') {
			opts->len = PRINTF_LEN_CHAR;
			fmt++;
		} else {
			opts->len = PRINTF_LEN_SHORT_INT;
		}
		break;
	// long
	case 'l':
		if (*fmt == 'l') {
			opts->len = PRINTF_LEN_LONG_LONG_INT;
			fmt++;
		} else {
			opts->len = PRINTF_LEN_LONG_INT;
		}
		break;
	// size_t
	case 'z':
		opts->len = PRINTF_LEN_SIZE_T;
		break;
	default:
		opts->len = PRINTF_LEN_INT;
		return;
	}

	*res = fmt;
}

static void get_radix(char spec, options_t *opts)
{
	switch (spec) {
	case 'x':
	case 'X':
		opts->radix = 16;
		break;
	case 'o':
		opts->radix = 8;
		break;
	default:
		opts->radix = 10;
		break;
	}
}

static void get_case(char spec, options_t *opts)
{
	if (spec == 'X')
		opts->is_uppercase = 1;
}

static char printf_itoc(int uppercase, int i)
{
	// decimal
	if (i < 10) {
		return i + '0';
	}
	// hex
	if (uppercase) {
		return (i - 10) + 'A';
	} else {
		return (i - 10) + 'a';
	}
}

static int printf_lltoa(char *buf, options_t *opts, bool is_neg,
						unsigned long long int num)
{
	int precision = 0;
	char *start = buf;

	// get width of number
	int len = 0;
	unsigned long long int temp = num;
	if (temp == 0)
		len = 1;
	while (temp) {
		if (opts->precision_set && precision++ >= opts->precision)
			break;
		temp /= opts->radix;
		len++;
	}
	precision = 0;

	// sign
	if (is_neg) {
		*(buf++) = '-';
	} else if (opts->sign) {
		*(buf++) = '+';
	} else if (opts->space) {
		*(buf++) = ' ';
	}

	// radix specifier
	if (opts->hash) {
		if (opts->radix == 8) {
			*(buf++) = '0';
			*(buf++) = 'o';
		}
		if (opts->radix == 16) {
			*(buf++) = '0';
			*(buf++) = 'x';
		}
	}


	// print zeros if needed
	if (opts->width_set && len < opts->width && opts->zero) {
		while(len++ < opts->width)
			*(buf++) = '0';
	}

	// write number
	if (num == 0) {
		*(buf++) = '0';
	}
	while (num) {
		if (opts->precision_set && precision++ >= opts->precision)
			break;
		*(buf++) = printf_itoc(opts->is_uppercase, num % opts->radix);
		num /= opts->radix;
	}
	*(buf++) = '\0';

	return buf - start;
}

static void handle_int_specifier(context_t *ctx, options_t *const opts,
								 bool has_sign_bit, data_t num)
{
	bool is_neg = false;

	// get sign if possible neg
	if (has_sign_bit) {
		if (num.i < 0) {
			num.i = -num.i;
			is_neg = true;
		}
	}

	// get length of number and number
	char buf[PRINTF_NUMERIC_BUF_LEN];
	int buf_len = printf_lltoa(buf, opts, is_neg, num.u);

	// get needed padding
	int padding = 0;
	if (opts->width_set && (buf_len < opts->width))
		padding = opts->width - buf_len;

	/* print */
	// left padding
	if (opts->left == 0) {
		for (int i = 0; i < padding; i++)
			printf_putc(ctx, opts->zero ? '0' : ' ');
	}
	// number
	for (int i = 0; i < buf_len; i++)
		printf_putc(ctx, buf[i]);
	// right padding
	if (opts->left == 1) {
		for (int i = 0; i < padding; i++)
			printf_putc(ctx, opts->zero ? '0' : ' ');
	}
}

static void handle_char_specifier(context_t *ctx, data_t c)
{
	printf_putc(ctx, c.c);
}

static void handle_string_specifier(context_t *ctx, options_t *opts, data_t data)
{
	char *str = data.str;
	int str_len = 0;

	// get length of string
	if (opts->precision_set)
		str_len = opts->precision;
	else
		str_len = strlen(str);

	// get needed padding
	int padding = 0;
	if (opts->width_set && (str_len < opts->width))
		padding = opts->width - str_len;

	/* print */
	// left padding
	if (opts->left == 0) {
		for (int i = 0; i < padding; i++)
			printf_putc(ctx, ' ');
	}
	// string
	for (int i = 0; i < str_len; i++)
		printf_putc(ctx, str[i]);
	// right padding
	if (opts->left == 1) {
		for (int i = 0; i < padding; i++)
			printf_putc(ctx, ' ');
	}
}

static void do_printf(context_t *ctx, va_list args)
{
	const char *fmt = ctx->format;

	char c;
	while (c = *fmt++, c != '\0') {
		// save start of fmt for current iteration
		const char *start = fmt - 1;

		// ignore if not %
		if (c != '%') {
			printf_putc(ctx, c);
			continue;
		}

		// read opts
		options_t opts = { 0 };
		while (parse_flag(&fmt, &opts))
			;
		parse_width(&fmt, &opts);
		parse_precision(&fmt, &opts);
		parse_length(&fmt, &opts);

		// read specifier
		char spec = *fmt++;
		get_radix(spec, &opts);
		get_case(spec, &opts);

		// read varied width / precision
		if (opts.width_varies) {
			opts.width_set = 1;
			opts.width = va_arg(args, int);
		}
		if (opts.precision_varies) {
			opts.precision_set = 1;
			opts.precision = va_arg(args, int);
		}
		// read data from args
		data_t data;
		switch (spec) {
		case 'p':
			opts.len = PRINTF_LEN_SIZE_T;
			opts.width_set = true;
			opts.radix = 16;
			opts.hash = true;
			opts.zero = true;
		case 'd':
		case 'i':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			// read number from arg
			switch (opts.len) {
			case PRINTF_LEN_CHAR:
				data.u = va_arg(args, unsigned int); // char
				break;
			case PRINTF_LEN_SHORT_INT:
				data.u = va_arg(args, unsigned int); // short int
				break;
			case PRINTF_LEN_INT:
				data.u = va_arg(args, unsigned int);
				break;
			case PRINTF_LEN_LONG_INT:
				data.u = va_arg(args, unsigned long int);
				break;
			case PRINTF_LEN_LONG_LONG_INT:
				data.u = va_arg(args, unsigned long long int);
				break;
			case PRINTF_LEN_SIZE_T:
				data.u = va_arg(args, size_t);
				break;
			}
			break;
			// end int
		case 's':
			// read string
			data.str = va_arg(args, void*);
			break;
			// end string
		case 'c':
			// read char
			data.c = va_arg(args, int);
			break;
			// end char
		}

		switch (spec) {
		// signed int
		case 'd':
		case 'i':
			handle_int_specifier(ctx, &opts, true, data);
			break;
		// unsigned int
		case 'p':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			handle_int_specifier(ctx, &opts, false, data);
			break;
		// character
		case 'c':
			handle_char_specifier(ctx, data);
			break;
		// string
		case 's':
			handle_string_specifier(ctx, &opts, data);
			break;
		// unknown
		default:
			// print from % to current
			for (; start < fmt; start++)
				printf_putc(ctx, *start);
			break;
		}
	}
}

void printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

size_t sprintf(char *restrict s, const char *format, ...)
{
	va_list args;
	size_t amt;
	va_start(args, format);
	amt = vsprintf(s, format, args);
	va_end(args);
	return amt;
}

size_t snprintf(char *restrict s, size_t maxlen, const char *format, ...)
{
	va_list args;
	size_t amt;
	va_start(args, format);
	amt = vsnprintf(s, maxlen, format, args);
	va_end(args);
	return amt;
}

void vprintf(const char *format, va_list args)
{
	vfprintf(stdout, format, args);
}

size_t vsprintf(char *restrict s, const char *format, va_list args)
{
	// create context
	context_t ctx = { 0 };
	ctx.format = format;
	// sprintf buffer
	ctx.out.buf = s;
	ctx.to_file = 0;
	// print
	do_printf(&ctx, args);
	return ctx.written_len;
}

size_t vsnprintf(char *restrict s, size_t maxlen, const char *format,
				 va_list args)
{
	// create context
	context_t ctx = { 0 };
	ctx.format = format;
	// sprintf buffer
	ctx.out.buf = s;
	ctx.to_file = 0;
	// sprintf max_len
	ctx.has_max_len = 1;
	ctx.max_len = maxlen;
	// print
	do_printf(&ctx, args);
	return ctx.written_len;
}

void fprintf(FILE *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stream, format, args);
	va_end(args);
}

void vfprintf(FILE *stream, const char *format, va_list args)
{
	// create context
	context_t ctx = { 0 };
	ctx.format = format;
	// fprintf stream
	ctx.out.file = stream;
	ctx.to_file = 1;
	// print
	do_printf(&ctx, args);
}

void putc(char c) {
	fputc(stdout, c);
}

void puts(const char *str) {
	fputs(stdout, str);
}

void fputs(FILE *stream, const char *s) {
	while (*s)
		fputc(stream, *s++);
}
