#include <string.h>
#include <stdlib.h>
#include <utmpx.h>
#include "errors.h"
#include "userlist.h"

static void ul_internal_resize(char ***list, size_t *size);

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
		free(l->array[i]);
	for(int i = 0; (tmp = getutxent()) != NULL;)
	{
		if(tmp->ut_type == USER_PROCESS)
		{
			if(i + 1 == (int)l->length) // If the next item would be the end of the list, resize (remember, we want it to always be NULL-terminated)
			{
				ul_internal_resize(&l->array, &l->length);
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

static void ul_internal_resize(char ***list, size_t *size)
{
	*size *= 2;
	void *tmp = realloc(*list, *size * sizeof(char*));
	if(tmp == NULL)
		fatalperror("realloc");
	*list = tmp;
}
