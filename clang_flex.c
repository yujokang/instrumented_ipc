#include "flex_basic.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TEST_OPTION "-v"

#define COMMAND MACRO_TO_STRING(BARE_COMMAND)
#define COMMAND_XX COMMAND "++"

static const char *not_link[] = {"-c", "-S", "-E", "-shared"};

static int check_linking(int argc, char **argv)
{
	if (argc == 1 && argv[1] != NULL && strcmp(argv[1], TEST_OPTION) == 0) {
		return 0;
	} else {
		int arg_i;

		for (arg_i = 0; arg_i < argc; arg_i++) {
			char *arg = argv[arg_i];
			unsigned not_link_i;

			for (not_link_i = 0;
			     not_link_i < sizeof(not_link) /
					  sizeof(not_link[0]);
			     not_link_i++) {
				if (strcmp(arg, not_link[not_link_i]) == 0) {
					return 0;
				}
			}
		}
	}

	return 1;
}

enum {
	EXTRA_XCLANG_LOAD, EXTRA_LOAD, EXTRA_XCLANG_SO, EXTRA_SO,
#ifdef BUILD_DEBUG
	EXTRA_G,
#endif /* BUILD_DEBUG */
	N_EXTRAS
};

#define EXTRA_XCLANG_VALUE "-Xclang"
#define EXTRA_G_VALUE "-g"
static char *
extra_values[N_EXTRAS] = {EXTRA_XCLANG_VALUE, "-load",
			  EXTRA_XCLANG_VALUE, MACRO_TO_STRING(EXTRA_SO_VALUE),
#ifdef BUILD_DEBUG
				EXTRA_G_VALUE,
#endif /* BUILD_DEBUG */
				};

#define XX_MARKER "++"
#define XX_MARKER_LEN strlen(XX_MARKER)

static char *get_real_command(char *original_command)
{
	size_t original_len = strlen(original_command);
	if (original_len > XX_MARKER_LEN) {
		size_t maybe_marker_start = original_len - XX_MARKER_LEN;

		if (strcmp(original_command + maybe_marker_start,
			   XX_MARKER) == 0) {
			return COMMAND_XX;
		}
	}
	return COMMAND;
}

static char **
create_new_argv(int old_argc, char **old_argv, int linking)
{
	int new_argc_without_link = old_argc + N_EXTRAS;
	int new_argc = new_argc_without_link + (linking ? 2 : 0);
	char **new_argv = malloc(sizeof(char *) * (new_argc + 1));
	int arg_i, extra_i;

	if (new_argv == NULL) {
		perror("Failed to allocate new arguments");
		return NULL;
	}

	new_argv[0] = get_real_command(old_argv[0]);
	for (arg_i = 1; arg_i < old_argc; arg_i++) {
		new_argv[arg_i] = old_argv[arg_i];
	}
	for (extra_i = 0; extra_i < N_EXTRAS; extra_i++) {
		new_argv[extra_i + old_argc] = extra_values[extra_i];
	}
	if (linking) {
		append_objs(new_argv, new_argc_without_link);
	}

	new_argv[new_argc] = NULL;
	return new_argv;
}

int main(int argc, char *argv[])
{
	int linking = check_linking(argc, argv);
	char **new_argv = create_new_argv(argc, argv, linking);
	if (new_argv == NULL) {
		return -1;
	}

	execv(get_real_command(argv[0]), new_argv);

	perror("Failed to execute compiler");
	return -1;
}
