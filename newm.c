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

int main()
{
	/* Flags */
	bool f_oneshot = true;
	bool f_forking = true;
	bool f_initialshow = true;
	bool f_showins = true;
	bool f_showouts = true;

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
	if(f_initialshow)
	{
		/* TODO */
	}


	/* If we are forking, fork() and then exit the parent */
	if(f_forking)
	{
		pid_t pid = fork();
		if(pid > 0)
			exit(0);
		else if(pid == -1)
			fatalperror("fork");
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
		if(f_forking)
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
			if(f_showouts)
				out_message(r);
		}
		else
		{
			char *r = ul_subtract(afterlist, beforelist);
			if(r == NULL)
				continue;
			if(f_showins)
				in_message(r);
		}

		if(f_oneshot)
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
	printf("\e[1m%s logged in.\e[0m\n", name);
}

void out_message(char *name)
{
	printf("\e[1m%s logged out.\e[0m\n", name);
}
