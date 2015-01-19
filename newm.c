#include <stdio.h>
#include <paths.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include "errors.h"
#include "userlist.h"
#define FAIL	EXIT_FAILURE
#define SUCCESS	EXIT_SUCCESS

void free_mem_on_exit(void);
void watch_and_wait(int,int);
void in_message(char*);
void out_message(char*);

struct userlist *beforelist, *afterlist;

struct {
	unsigned int oneshot :1;
	unsigned int forking :1;
	unsigned int initialshow :1;
	unsigned int showins :1;
	unsigned int showouts :1;
} config = {
	.oneshot = false,
	.forking = true,
	.initialshow = true,
	.showins = true,
	.showouts = true
};

int main()
{
	/* Variables */
	int fd;
	int stdout_fileno;

	/* Initialize variables */
	afterlist = ul_create(8);
	beforelist = ul_create(8);
	stdout_fileno = fileno(stdout);

	/* Set up atexit */
	atexit(free_mem_on_exit);

	/* Read conf file */
	/* TODO */

	/* If we are supposed to print a user list upon startup, do it now, before fork()ing */
	if(config.initialshow)
	{
		struct userlist *ls = ul_create(8);
		ul_populate(ls);
		ul_sort(ls);
		printf("Users logged in: ");
		for(int i = 0; ls->array[i] != NULL; i++)
		{
			if(i > 0 && !strcmp(ls->array[i], ls->array[i-1]))
				continue;
			printf("%s, ", ls->array[i]);
		}
		printf("\b\b  \n");
		ul_free(ls);
	}

	/* If we are forking, fork() and then exit the parent */
	if(config.forking)
	{
		pid_t pid = fork();
		if(pid > 0)
			exit(0);
		else if(pid == -1)
			fatalperror("fork");
		/* This setpgid() call changes the process-group ID so 'w' reports the shell (not us!) as the current command */
		setpgid(getpid(),getpid());
	}

	/* Start and setup inotify */
	fd = inotify_init();
	if(fd < 0)
		fatalperror("inotify_init");
	if(inotify_add_watch(fd, _PATH_UTMP, IN_MODIFY) < 0)
		fatalperror("inotify_add_watch");

	while(1)
	{
		ul_populate(beforelist);

		/* If we are fork()ing, we want to monitor stdout, which requires us to use select() with a timeout */
		if(config.forking)
		{
			watch_and_wait(fd, stdout_fileno);
		}

		struct inotify_event evt;
		if(read(fd, &evt, sizeof(struct inotify_event)) < 0)
			fatalperror("read");

		ul_populate(afterlist);

		int firstlen = ul_count(beforelist);
		int secondlen = ul_count(afterlist);

		if(firstlen == secondlen)
		{
			continue;
		}
		else if(firstlen > secondlen)
		{
			char *r = ul_subtract(beforelist, afterlist);
			if(r == NULL)
				continue;
			if(config.showouts)
				out_message(r);
		}
		else
		{
			char *r = ul_subtract(afterlist, beforelist);
			if(r == NULL)
				continue;
			if(config.showins)
				in_message(r);
		}

		if(config.oneshot)
		{
			exit(0);
		}
	}

	exit(SUCCESS);
}

void free_mem_on_exit(void)
{
	ul_free(beforelist);
	ul_free(afterlist);
}


void watch_and_wait(int inotifyfd, int stdoutfd)
{
	while(1)
	{
		fd_set read;
		struct timeval watchtimeout;
		FD_ZERO(&read);
		FD_SET(inotifyfd, &read);
		watchtimeout.tv_sec = 30;
		watchtimeout.tv_usec = 0;
		int ret = select(inotifyfd+1, &read, NULL, NULL, &watchtimeout);
		if(ret < 0)
		{
			fatalperror("select");
		}
		if(!isatty(stdoutfd))
			exit(0);
		if(ret > 0)
			break;
	}
}

void in_message(char *name)
{
	printf("\a\n\n\e[31m\e[1m%s just logged in!\e[0m\n\n", name);
}

void out_message(char *name)
{
	printf("\n\n\e[1m%s logged out.\e[0m\n\n", name);
}
