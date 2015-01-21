#include <stdio.h>
#include <paths.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include "errors.h"
#include "userlist.h"
#define FAIL	EXIT_FAILURE
#define SUCCESS	EXIT_SUCCESS

void free_mem_on_exit(void);
void watch_and_wait(int,int);
void in_message(char*);
void out_message(char*);
void on_login(char*);
void on_logout(char*);
void run_command(char*);

struct userlist *beforelist, *afterlist;

struct {
	unsigned int oneshot :1;
	unsigned int forking :1;
	unsigned int initialshow :1;
	unsigned int listen_ins :1;
	unsigned int listen_outs :1;
	char *login_command;
	char *login_message;
	char *logout_command;
	char *logout_message;
} config = {
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

int main()
{
	/* Variables */
	int fd;
	int stdout_fileno;
	char *username;

	/* Initialize variables */
	afterlist = ul_create(8);
	beforelist = ul_create(8);
	stdout_fileno = fileno(stdout);

	/* Get our username */
	struct passwd *p = getpwuid(getuid());
	if(p == NULL)
		fatalperror("getpwuid");
	/* warning - username will now point to a static area, subsequent getpwuid calls may overwite it */
	username = p->pw_name;

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

	/* If we aren't supposed to listen to INs *or* OUTs, no point in continuing */
	if(!config.listen_ins && !config.listen_outs)
		exit(EXIT_SUCCESS);

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

	/* Set up child-reaping for login-command */
	signal(SIGCHLD, SIG_IGN);

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
			if(!strcmp(r, username))
				continue;
			if(config.listen_outs)
				on_logout(r);
		}
		else
		{
			char *r = ul_subtract(afterlist, beforelist);
			if(r == NULL)
				continue;
			if(!strcmp(r, username))
				continue;
			if(config.listen_ins)
				on_login(r);
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

void run_command(char *cmd)
{
	pid_t pid = fork();
	if(pid == 0) /* Child process */
	{
		system(cmd);
		exit(EXIT_SUCCESS);
	}
	else if(pid == -1) /* Error */
	{
		fatalperror("fork");
	}
	else /* Parent, after child was successfully started */
	{
		waitpid((pid_t)-1, NULL, WNOHANG);
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

void on_logout(char *name)
{
	if(config.logout_message != NULL)
		out_message(name);
	if(config.logout_command != NULL)
		run_command(config.logout_command);
}

void on_login(char *name)
{
	if(config.login_message != NULL)
		in_message(name);
	if(config.login_command != NULL)
		run_command(config.login_command);
}
