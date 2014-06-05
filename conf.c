#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "conf.h"

void conf_parse(confopt *c, char *f)
{
	FILE *fp = fopen(f, "r");
	if(fp == NULL)
	{
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	char *line = NULL;
	size_t len = 0;
	int line_number = 0;
	while(getline(&line, &len, fp) != -1)
	{
		char *origline = line;
		line_number++;
		chomp(line);
		for(; isblank(line[0]); line++); //Move past blank characters
		if(line[0] == 0 || line[0] == '#')
		{
			line = origline;
			continue; // A comment or end of line
		}
		int i;
		for(i = 0; isalnum(line[i]) || line[i] == '_'; i++);
		char *name = malloc(i + 1); // Get memory for name + terminating NULL
		if(name == NULL)
		{
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		strncpy(name, line, i);
		name[i] = 0;
		line += i;
		for(; isblank(line[0]); line++); //Move past blank characters
		if(line[0] != '=')
		{
			fprintf(stderr, "Syntax error in '%s', line %d\n", f, line_number);
			exit(EXIT_FAILURE);
		}
		line++; // Move past equal sign
		for(; isblank(line[0]); line++); //Move past blank characters
		for(i = 0; line[i] != 0; i++); //Count the bytes in the value
		char *value = malloc(i + 1); // Get memory for value + terminating NULL
		if(value == NULL)
		{
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		strncpy(value, line, i);
		value[i] = 0;
		bool assigned = false;
		for(int it = 0; c[it].name != NULL; it++)
		{
			if(!strcmp(c[it].name, name))
			{
				*c[it].val = value;
				assigned = true;
				break;
			}
		}
		if(!assigned)
		{
			fprintf(stderr, "conf WARNING -- unused option '%s'\n", name);
		}
		free(name);
		line = origline;
	}
	free(line);
	fclose(fp);
	for(int i = 0; c[i].name != NULL; i++)
	{
		if(*c[i].val == NULL && c[i].defval != NULL)
		{
			int t = strlen(c[i].defval);
			char *r = malloc(t+1); // The program expects this to be malloc'd memory, let's not let it down
			if(r == NULL)
			{
				perror("malloc");
				exit(EXIT_FAILURE);
			}
			strncpy(r, c[i].defval, t);
			r[t] = 0;
			*c[i].val = r;
		}
	}
}

char *chomp(char *buf)
{
	char *c = buf + strlen(buf) - 1;
	if(*c == '\n')
		*c = 0;
	c--;
	if(*c == '\r')
		*c = 0;
	return buf;
}
