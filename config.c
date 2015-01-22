#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "errors.h"
#include "config.h"

#define fmterr()	do{fprintf(stderr,"Format error in configuration file %s on line %d\n", filename, lineno);}while(0)

const struct _config defconfig = {
		.oneshot = false,
		.forking = true,
		.initialshow = true,
		.listen_ins = true,
		.listen_outs = true,
		.login_command = NULL,
		.login_message = "",
		.logout_command = NULL,
		.logout_message = ""
};

bool str_to_bool(char*);
char *chomp(char*);
void insert_option(char*, char*, struct _config*);

struct _config *load_config(char *filename)
{
	static struct _config cfg;
	cfg = defconfig;
	FILE *fp = fopen(filename, "r");
	if(errno == ENOENT)
		return &cfg;
	if(fp == NULL)
		fatalperror("fopen");
	char *line = NULL;
	size_t len = 0;
	int line_number = 0; // UNUSED?
	while(getline(&line, &len, fp) != -1)
	{
		char *origline = line;
		line_number++;
		chomp(line);
		// OPTIONAL Move past whitespace
		if(line[0] == 0 || line[0] == '#')
		{
			line = origline;
			continue; // Empty line/Comment
		}
		int i;
		for(i = 0; isalnum(line[i]) || line[i] == '-'; i++);
		char *name = malloc(i + 1); // Get memory for name + terminating NULL
		if(name == NULL)
			fatalperror("malloc");
		strncpy(name, line, i);
		name[i] = 0;
		line += i;
		// OPTIONAL Move past whitespace
		if(line[0] != '=')
			fatalerror("Syntax error in configuration file.\n");
		line++; // Move past equal sign
		// OPTIONAL Move past whitespace
		for(i = 0; line[i] != 0; i++); // Count the bytes in the value
		char *value = malloc(i + 1); // Get memory for value + terminating NULL
		if(value == NULL)
			fatalperror("malloc");
		strncpy(value, line, i);
		value[i] = 0;
		// use name/value
		insert_option(name, value, &cfg);
		line = origline;
	}
	free(line);
	fclose(fp);
	return &cfg;
}

/*struct _config *load_config(char *filename)
{
#define skip_white()	while(isspace(tmp[idx])){idx++}
	static struct _config ret;
	ret = defconfig;
	FILE *c = fopen(filename, "r");
	if(c == NULL)
		fatalperror("fopen");
	char *line = malloc(64);
	if(line == NULL)
		fatalperror("malloc");
	ssize_t bytesread;
	size_t n = 64;
	unsigned int lineno = 0;
	while((bytesread = getline(&line, &n, c)) > 0)
	{
		char *tmp = line;
		unsigned int idx = 0;
		size_t linelen = strlen(tmp);		
		chomp(tmp, &linelen); // Remove trailing newline

		if(linelen == 0) // Skip if empty line
			continue;

		if(tmp[0] == '#') // Skip this line if it is a comment
			continue;

		for(idx = 0; tmp[idx] != 0; idx++)
		{

		}

		lineno++;
	}
	free(line);
	fclose(c);
	if(bytesread == -1)
		fatalperror("getline");
	return &ret;
}*/

void insert_option(char *key, char *value, struct _config *cfg)
{
	if(!strcmp(key, "login-command"))
	{
		cfg->login_command = value;
	}
	else if(!strcmp(key, "logout-command"))
	{
		cfg->logout_command = value;
	}
	else if(!strcmp(key, "login-message"))
	{
		cfg->login_message = value;
	}
	else if(!strcmp(key, "logout-message"))
	{
		cfg->logout_message = value;
	}
	else if(!strcmp(key, "oneshot"))
	{
		cfg->oneshot = str_to_bool(value);
	}
	else if(!strcmp(key, "forking"))
	{
		cfg->forking = str_to_bool(value);
	}
	else if(!strcmp(key, "initialshow"))
	{
		cfg->initialshow = str_to_bool(value);
	}
	else if(!strcmp(key, "listen"))
	{
		if(!strcmp(value, "all"))
		{
			cfg->listen_ins = true;
			cfg->listen_outs = true;
		}
		else if(!strcmp(value, "none"))
		{
			cfg->listen_ins = false;
			cfg->listen_outs = false;
		}
		else if(!strcmp(value, "ins"))
		{
			cfg->listen_ins = true;
			cfg->listen_outs = false;
		}
		else if(!strcmp(value, "outs"))
		{
			cfg->listen_ins = false;
			cfg->listen_outs = true;
		}
		else
		{
			nonfatalerror("Unknown value for 'listen', should be one of all,none,ins,outs.\n");
		}
	}
	(void)cfg;
}

bool str_to_bool(char *str)
{
	if(!strcmp(str, "yes") || !strcmp(str, "true") || !strcmp(str, "1"))
		return true;
	else if(!strcmp(str, "no") || !strcmp(str, "false") || !strcmp(str, "0"))
		return false;
	else
		nonfatalerror("Invalid boolean value in configuration file.\n");
	return false;
}

char *chomp(char *buf)
{
    char *c = buf + strlen(buf) - 1;
    if(*c == '\n')
	{
        *c = 0;
	    c--;
	    if(*c == '\r')
		{
	        *c = 0;
		}
	}
    return buf;
}
