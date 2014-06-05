#ifndef __CONF_H__
#define __CONF_H__
#define confopt_END	{NULL,NULL,NULL}
typedef struct {
	char *name;		/* Name of option */
	char *defval;	/* Default value of option */
	char **val;		/* Pointer to variable to store the value of the option in */
} confopt;
void conf_parse(confopt*, char*);
char *chomp(char*);
#endif
