#include <string.h>
#include <stdlib.h>
#include <utmpx.h>
#include <stdbool.h>
#include "errors.h"
#include "userlist.h"

struct userlist *ul_create(int initial_length)
{
	struct userlist *ret = malloc(sizeof(struct userlist));
	if(!ret)
		fatalperror("malloc");
	ret->array = malloc(initial_length * sizeof(char*));
	if(!ret->array)
		fatalperror("malloc");
	ret->array[0] = NULL;
	ret->length = initial_length;
	return ret;
}

void ul_populate(struct userlist *l)
{
	struct utmpx *tmp;
	for(int i = 0; l->array[i] != NULL; i++)
	{
		free(l->array[i]);
		l->array[i] = NULL;
	}
	for(int i = 0; (tmp = getutxent()) != NULL;)
	{
		if(tmp->ut_type == USER_PROCESS)
		{
			if(i + 1 == (int)l->length) // If the next item would be the end of the list, resize (remember, we want it to always be NULL-terminated)
			{
				char **tmp; // We need to do it in two steps so freemem (atexit) can free l->array even if realloc() failed
				l->length *= 2;
				if((tmp = realloc(l->array, l->length * sizeof(char*))) == NULL)
					fatalperror("realloc");
				l->array = tmp;
			}
			l->array[i] = malloc(strlen(tmp->ut_user) + 1);
			if(l->array[i] == NULL)
				fatalperror("malloc");
			strcpy(l->array[i], tmp->ut_user);
			l->array[++i] = NULL;
		}
	}
	endutxent();
}

void ul_sort(struct userlist *l)
{
	bool done = false;
	if(l->array[0] == NULL)
		return;
	while(!done)
	{
		done = true;
		for(int i = 1; l->array[i] != NULL; i++)
		{
			if(strcmp(l->array[i-1], l->array[i]) > 0)
			{
				char *tmp = l->array[i-1];
				l->array[i-1] = l->array[i];
				l->array[i] = tmp;
				done = false;
			}
		}
	}
}

size_t ul_count(struct userlist *l)
{
	int i;
	for(i = 0; l->array[i] != NULL; i++);
	return i;
}

char *ul_subtract(struct userlist *bigger, struct userlist *smaller)
{
	int n = 0;
	int found = 0;
	char **biglist = bigger->array;
	char **smalllist = smaller->array;
	for(n = 0; biglist[n] != NULL; n++)
	{
		found = 0;
		for(int i = 0; smalllist[i] != NULL; i++)
		{
			if(!strcmp(smalllist[i], biglist[n]))
			{
				found = 1;
				break;
			}
		}
		if(!found)
		{
			return biglist[n];
		}
	}
	return NULL;
}

void ul_free(struct userlist *l)
{
	if(!l)
		return;
	if(l->array)
	{
		for(int i = 0; l->array[i] != NULL; i++)
			free(l->array[i]);
		free(l->array);
	}
	free(l);
}
