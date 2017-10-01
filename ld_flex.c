#include "flex_basic.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TO_STRING(value) #value
#define MACRO_TO_STRING(macro) TO_STRING(macro)

#define COMMAND MACRO_TO_STRING(BARE_COMMAND)

static char **
create_new_argv(int old_argc, char **old_argv)
{
	int new_argc = old_argc + 1;
	char **new_argv = malloc(sizeof(char *) * (new_argc + 1));
	int arg_i;

	if (new_argv == NULL) {
		perror("Failed to allocate new arguments");
		return NULL;
	}
	new_argv[0] = COMMAND;
	for (arg_i = 1; arg_i < old_argc; arg_i++) {
		new_argv[arg_i] = old_argv[arg_i];
	}
	append_objs(new_argv, old_argc);

	return new_argv;
}

int main(int argc, char *argv[])
{
	char **new_argv = create_new_argv(argc, argv);
	if (new_argv == NULL) {
		return -1;
	}

	execv(COMMAND, new_argv);

	perror("Failed to execute compiler");
	return -1;
}
