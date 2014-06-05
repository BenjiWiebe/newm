#include <stdio.h>
#include <utmpx.h>
#include <paths.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#define FAIL	EXIT_FAILURE
#define SUCCESS	EXIT_SUCCESS
#define dbg printf
#define TIMEOUT	1

struct list {
	size_t length;
	char **array;
};

void resize(char***,size_t*);
char *strsubtract(char**,char**);
void fatalperror(char*);
void fatalerror(char*);
struct list *list_create(int);
void list_populate(struct list*);
size_t list_count(struct list*);
void list_free(struct list*);
void in_message(char*);
void out_message(char*);

struct list beforelist, afterlist;

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

struct list *list_create(int initial_length)
{
	struct list *ret = malloc(sizeof(struct list));
	if(!ret)
		fatalperror("malloc");
	ret->array = malloc(initial_length * sizeof(char*));
	if(!ret->array)
		fatalperror("malloc");
	ret->array[0] = NULL;
	ret->length = initial_length;
	return ret;
}

void list_populate(struct list *l)
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
				resize(&l->array, &l->length);
			}
			l->array[i] = malloc(strlen(tmp->ut_user) + 1);
			if(l->array[i] == NULL)
				fatalperror("malloc");
			strcpy(l->array[i], tmp->ut_user);
			l->array[++i] = NULL; //TODO FIXME DOUBLE CHECK THIS CODE!!
		}
	}
	endutxent();
}

size_t list_count(struct list *l)
{
	int i;
	for(i = 0; l->array[i] != NULL; i++);
	return i;
}

void list_free(struct list *l)
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

void resize(char ***list, size_t *size)
{
	//int oldsize = *size;
	*size *= 2;
	void *tmp = realloc(*list, *size * sizeof(char*));
	if(tmp == NULL)
		fatalperror("realloc");
	*list = tmp;
	//memset(tmp + oldsize, 0, oldsize); // Zero the new memory
}

int main()
{
	/* Flags */
	bool f_oneshot = true;
	bool f_forking = true;
	bool f_initialshow = true;

	/* Variables */
	fd_set watch;
	struct timeval watchtimeout;
	struct list *beforelist, *afterlist;
	int fd;
	int stdout_fileno;

	/* Initialize variables */
	afterlist = list_create(8);
	beforelist = list_create(8);
	stdout_fileno = fileno(stdout);

	/* Read conf file */
	/* TODO */

	/* If we are supposed to print a user list upon startup, do it now, before fork()ing */
	if(f_initialshow)
	{
		/* TODO */
	}


	/* If we are forking, fork() and then exit the parent */
	if(f_forking)
	{
		pid_t pid = fork();
		dbg("Forked!\n");
		if(pid > 0)
			exit(0);
		else if(pid == -1)
			fatalperror("fork");
		dbg("Child.\n");
	}

	/* Start and setup inotify */
	fd = inotify_init();
	if(fd < 0)
		fatalperror("inotify_init");
	if(inotify_add_watch(fd, _PATH_UTMP, IN_MODIFY) < 0)
		fatalperror("inotify_add_watch");

	while(1)
	{
		list_populate(beforelist);

		/* FIXME if the below fixme gets implemented, why not move all the select() stuff into a function, to make the main loop cleaner? */
		/* FIXME Could we maybe use select() all the time, and set the watchtimeout pointer to NULL when not fork()ing??? */
		/* If we are fork()ing, we want to monitor stdout, which requires us to use select() with a timeout */
		if(f_forking)
		{
			dbg("Watching and waiting...\n");
			while(1)
			{
				FD_ZERO(&watch);
				FD_SET(fd, &watch);
				watchtimeout.tv_usec = 0;
				watchtimeout.tv_sec = TIMEOUT;
				int ret = select(fd+1, &watch, NULL, NULL, &watchtimeout);
				if(ret == 0)
				{
					if(!isatty(stdout_fileno))
						exit(0);
				}
				else if(ret > 0)
				{
					break;
				}
				else
				{
					fatalperror("select");
				}
			}
		}

		struct inotify_event evt;
		if(read(fd, &evt, sizeof(struct inotify_event)) < 0)
			fatalperror("read");

		list_populate(afterlist);

		int firstlen = list_count(beforelist);
		int secondlen = list_count(afterlist);

		if(firstlen == secondlen)
		{
			continue;
		}
		else if(firstlen > secondlen)
		{
			char *r = strsubtract(beforelist->array, afterlist->array);
			if(r == NULL)
				continue;
			out_message(r);
		}
		else
		{
			char *r = strsubtract(afterlist->array, beforelist->array);
			if(r == NULL)
				continue;
			in_message(r);
		}

		if(f_oneshot)
		{
			list_free(beforelist);
			list_free(afterlist);
			exit(0);
		}
	}

	exit(SUCCESS);
}


char *strsubtract(char **bigarray, char **smallarray)
{
	int n = 0;
	int found = 0;
	for(n = 0; bigarray[n] != NULL; n++)
	{
		found = 0;
		for(int i = 0; smallarray[i] != NULL; i++)
		{
			if(!strcmp(smallarray[i], bigarray[n]))
			{
				found = 1;
				break;
			}
		}
		if(!found)
		{
			return bigarray[n];
		}
	}
	return NULL;
}

void in_message(char *name)
{
	printf("\e[1m%s logged in.\e[0m\n", name);
}

void out_message(char *name)
{
	printf("\e[1m%s logged out.\e[0m\n", name);
}
