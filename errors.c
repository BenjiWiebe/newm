#include <stdio.h>
#include <stdlib.h>
#include "errors.h"

void fatalperror(char *s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

void fatalerror(char *s)
{
	fprintf(stderr, "%s\n", s);
	exit(EXIT_FAILURE);
}

void nonfatalerror(char *s)
{
	fprintf(stderr, "%s\n", s);
}
