#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "errors.h"

void fatalperror(char *s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

static void print_error(char *s, va_list ap, bool do_exit, int exit_val)
{
	vfprintf(stderr, s, ap);
	if(do_exit)
		exit(exit_val);
}

void fatalerror(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	print_error(s, ap, true, EXIT_FAILURE);
	va_end(ap);
}

void nonfatalerror(char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	print_error(s, ap, false, 0);
	va_end(ap);
}
