/* vi: set sw=4 ts=4: */
/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
//config:config DC
//config:	bool "dc (4.2 kb)"
//config:	default y
//config:	help
//config:	Dc is a reverse-polish desk calculator which supports unlimited
//config:	precision arithmetic.
//config:
//config:config FEATURE_DC_LIBM
//config:	bool "Enable power and exp functions (requires libm)"
//config:	default y
//config:	depends on DC
//config:	help
//config:	Enable power and exp functions.
//config:	NOTE: This will require libm to be present for linking.

//applet:IF_DC(APPLET(dc, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_DC) += dc.o

//usage:#define dc_trivial_usage
//usage:       "EXPRESSION..."
//usage:
//usage:#define dc_full_usage "\n\n"
//usage:       "Tiny RPN calculator. Operations:\n"
//usage:       "+, add, -, sub, *, mul, /, div, %, mod, "IF_FEATURE_DC_LIBM("**, exp, ")"and, or, not, xor,\n"
//usage:       "p - print top of the stack (without popping),\n"
//usage:       "f - print entire stack,\n"
//usage:       "o - pop the value and set output radix (must be 10, 16, 8 or 2).\n"
//usage:       "Examples: 'dc 2 2 add p' -> 4, 'dc 8 8 mul 2 2 + / p' -> 16"
//usage:
//usage:#define dc_example_usage
//usage:       "$ dc 2 2 + p\n"
//usage:       "4\n"
//usage:       "$ dc 8 8 \\* 2 2 + / p\n"
//usage:       "16\n"
//usage:       "$ dc 0 1 and p\n"
//usage:       "0\n"
//usage:       "$ dc 0 1 or p\n"
//usage:       "1\n"
//usage:       "$ echo 72 9 div 8 mul p | dc\n"
//usage:       "64\n"

//#include "libbb.h"
//#include "common_bufsiz.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
enum { COMMON_BUFSIZE = 1024 };
#ifdef _MSC_VER
#define ALIGNED(x) __declspec(align(x))
#else
#define ALIGNED(m) __attribute__ ((__aligned__(m)))
#endif
char bb_common_bufsiz1[COMMON_BUFSIZE] ALIGNED(sizeof(long long));
# define ALIGN1
# define ALIGN2
# define ALIGN4
#define setup_common_bufsiz() ((void)0)

#if _WIN32 && \
	(!defined(__USE_MINGW_ANSI_STDIO) || !__USE_MINGW_ANSI_STDIO)
#define ENABLE_FEATURE_DC_LIBM 1
#define LL_FMT "I64"
#else
#define LL_FMT "ll"
#endif

#define MAXINT(T) (T)( \
	((T)-1) > 0 \
	? (T)-1 \
	: (T)~((T)1 << (sizeof(T)*8-1)) \
	)

#if 0
typedef unsigned data_t;
#define DATA_FMT ""
#elif 0
typedef unsigned long data_t;
#define DATA_FMT "l"
#else
typedef unsigned long long data_t;
#define DATA_FMT LL_FMT
#endif

/**/
struct globals {
	unsigned pointer;
	unsigned base;
	double stack[1];
} FIX_ALIASING;
enum { STACK_SIZE = (COMMON_BUFSIZE - offsetof(struct globals, stack)) / sizeof(double) };
#define G (*(struct globals*)bb_common_bufsiz1)
#define pointer   (G.pointer   )
#define base      (G.base      )
#define stack     (G.stack     )
#define INIT_G() do { \
	setup_common_bufsiz(); \
	base = 10; \
} while (0)

#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))

#ifndef HAVE_STRCHRNUL
char* strchrnul(const char *s, int c)
{
	while (*s != '\0' && *s != c)
		s++;
	return (char*)s;
}
#endif

char* skip_whitespace(const char *s)
{
	/* In POSIX/C locale (the only locale we care about: do we REALLY want
	 * to allow Unicode whitespace in, say, .conf files? nuts!)
	 * isspace is only these chars: "\t\n\v\f\r" and space.
	 * "\t\n\v\f\r" happen to have ASCII codes 9,10,11,12,13.
	 * Use that.
	 */
	while (*s == ' ' || (unsigned char)(*s - 9) <= (13 - 9))
		s++;

	return (char *) s;
}

char* is_prefixed_with(const char *string, const char *key)
{
#if 0	/* Two passes over key - probably slower */
	int len = strlen(key);
	if (strncmp(string, key, len) == 0)
		return string + len;
	return NULL;
#else	/* Open-coded */
	while (*key != '\0') {
		if (*key != *string)
			return NULL;
		key++;
		string++;
	}
	return (char*)string;
#endif
}
char* bb_get_chunk_from_file(FILE *file, int *end)
{
	int ch;
	unsigned idx = 0;
	char *linebuf = NULL;

	while ((ch = getc(file)) != EOF) {
		/* grow the line buffer as necessary */
		if (!(idx & 0xff))
			linebuf = realloc(linebuf, idx + 0x100);
		linebuf[idx++] = (char) ch;
		if (ch == '\0')
			break;
		if (end && ch == '\n')
			break;
	}
	if (end)
		*end = idx;
	if (linebuf) {
		// huh, does fgets discard prior data on error like this?
		// I don't think so....
		//if (ferror(file)) {
		//	free(linebuf);
		//	return NULL;
		//}
		linebuf = realloc(linebuf, idx + 1);
		linebuf[idx] = '\0';
	}
	return linebuf;
}
char* xmalloc_fgetline(FILE *file)
{
	int i;
	char *c = bb_get_chunk_from_file(file, &i);

	if (i && c[--i] == '\n')
		c[i] = '\0';
#if _WIN32
	if (i && c[--i] == '\r')
		c[i] = '\0';
#endif

	return c;
}

static void check_under(void)
{
	if (pointer == 0) {
		printf("stack underflow\n"); 
		exit(1);
  }
}

static void push(double a)
{
	if (pointer >= STACK_SIZE) {
		printf("stack overflow\n"); 
		exit(1);
  }
	stack[pointer++] = a;
}

static double pop(void)
{
	check_under();
	return stack[--pointer];
}

static void add(void)
{
	push(pop() + pop());
}

static void sub(void)
{
	double subtrahend = pop();

	push(pop() - subtrahend);
}

static void mul(void)
{
	push(pop() * pop());
}

#if ENABLE_FEATURE_DC_LIBM
static void power(void)
{
	double topower = pop();

	push(pow(pop(), topower));
}
#endif

static void divide(void)
{
	double divisor = pop();

	push(pop() / divisor);
}

static void mod(void)
{
	data_t d = pop();

	push((data_t) pop() % d);
}

static void and(void)
{
	push((data_t) pop() & (data_t) pop());
}

static void or(void)
{
	push((data_t) pop() | (data_t) pop());
}

static void eor(void)
{
	push((data_t) pop() ^ (data_t) pop());
}

static void not(void)
{
	push(~(data_t) pop());
}

static void set_output_base(void)
{
	static const char bases[] ALIGN1 = { 2, 8, 10, 16, 0 };
	unsigned b = (unsigned)pop();

	base = *strchrnul(bases, b);
	if (base == 0) {
		printf("error, base %u is not supported", b);
		base = 10;
	}
}

static void print_base(double print)
{
	data_t x, i;

	x = (data_t) print;
	if (base == 10) {
		if (x == print) /* exactly representable as unsigned integer */
			printf("%"DATA_FMT"u\n", x);
		else
			printf("%g\n", print);
		return;
	}

	switch (base) {
	case 16:
		printf("%"DATA_FMT"x\n", x);
		break;
	case 8:
		printf("%"DATA_FMT"o\n", x);
		break;
	default: /* base 2 */
		i = MAXINT(data_t) - (MAXINT(data_t) >> 1);
		/* i is 100000...00000 */
		do {
			if (x & i)
				break;
			i >>= 1;
		} while (i > 1);
		do {
			putchar('1' - !(x & i));
			i >>= 1;
		} while (i);
		putchar('\n');
	}
}

static void print_stack_no_pop(void)
{
	unsigned i = pointer;
	while (i)
		print_base(stack[--i]);
}

static void print_no_pop(void)
{
	check_under();
	print_base(stack[pointer-1]);
}

struct op {
	const char name[4];
	void (*function) (void);
};

static const struct op operators[] = {
#if ENABLE_FEATURE_DC_LIBM
	{"**",  power},
	{"exp", power},
	{"pow", power},
#endif
	{"%",   mod},
	{"mod", mod},
	{"and", and},
	{"or",  or},
	{"not", not},
	{"eor", eor},
	{"xor", eor},
	{"+",   add},
	{"add", add},
	{"-",   sub},
	{"sub", sub},
	{"*",   mul},
	{"mul", mul},
	{"/",   divide},
	{"div", divide},
	{"p", print_no_pop},
	{"f", print_stack_no_pop},
	{"o", set_output_base},
};

/* Feed the stack machine */
static void stack_machine(const char *argument)
{
	char *end;
	double number;
	const struct op *o;

 next:
	number = strtod(argument, &end);
	if (end != argument) {
		argument = end;
		push(number);
		goto next;
	}

	/* We might have matched a digit, eventually advance the argument */
	argument = skip_whitespace(argument);

	if (*argument == '\0')
		return;

	o = operators;
	do {
		char *after_name = is_prefixed_with(argument, o->name);
		if (after_name) {
			argument = after_name;
			o->function();
			goto next;
		}
		o++;
	} while (o != operators + ARRAY_SIZE(operators));

	printf("syntax error at '%s'", argument);
	exit(1);
}


int main(int argc, char **argv)
{
	INIT_G();

	argv++;
	if (!argv[0]) {
		/* take stuff from stdin if no args are given */
		char *line;
		while ((line = xmalloc_fgetline(stdin)) != NULL) {
			stack_machine(line);
			free(line);
		}
	} else {
		do {
			stack_machine(*argv);
		} while (*++argv);
	}
	return EXIT_SUCCESS;
}
