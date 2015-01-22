#ifndef __CONFIG_H_INC_
#define __CONFIG_H_INC_

/* Length of longest configuration option name plus terminating NULL */
#define CONFIG_LONGEST_OPTION	15

#include <stdbool.h>

struct _config {
	unsigned int oneshot :1;
	unsigned int forking :1;
	unsigned int initialshow :1;
	unsigned int listen_ins :1;
	unsigned int listen_outs :1;
	char *login_command;
	char *login_message;
	char *logout_command;
	char *logout_message;
};

struct _config *load_config(char*);

#endif
