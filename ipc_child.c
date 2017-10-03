#include "ipc_child.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define NAME_BUF_SIZE (sizeof(char) * (PATH_MAX + 1))

static char *get_program_path()
{
	char *program_path = malloc(NAME_BUF_SIZE);
	if (program_path == NULL) {
		return NULL;
	}

	if (readlink("/proc/self/exe", program_path, NAME_BUF_SIZE) < -1) {
		free(program_path);
		return NULL;
	}
	return program_path;
}

#define PATH_SEP '/'

static char *get_program_name(char *program_path)
{
	char *name = program_path, *current = program_path;

	while (*current != '\0') {
		if (*current == PATH_SEP) {
			name = current + 1;
		}
		current += 1;
	}
	return name;
}

static inline int is_end(const char *list_str, unsigned list_i)
{
	char list_char = list_str[list_i];

	return list_char == PROGRAM_LIST_DELIM ||
		list_char == '\0';
}

int find_in_list(const char *name, const char *list_str)
{
	int matched = 0;
	unsigned list_len = strlen(list_str), name_i = 0, list_i;
	for (list_i = 0; list_i < list_len + 1 && !matched; list_i++) {
		if (is_end(list_str, list_i) && name[name_i] == '\0') {
			matched = 1;
		} else if (list_str[list_i] != name[name_i]) {
			while (!is_end(list_str, list_i)) {
				list_i++;
			}
			name_i = 0;
		} else {
			name_i++;
		}
	}
	return matched;
}

static int should_track_this_program(const char *should_track_env)
{
	char *program_list = getenv(should_track_env);
	if (program_list == NULL) {
		return 1;
	} else {
		char *program_path = get_program_path(),
			*program_name = get_program_name(program_path);
		int should_track = find_in_list(program_name, program_list);
		free(program_path);
		return should_track;
	}
}

void init_child(volatile struct shared_basics *state, int *init_ptr,
		const char *id_env, const char *should_track_env,
		int (*custom_setup)(volatile struct shared_basics *state))
{
	if (!(*init_ptr)) {
		char *id_str = getenv(id_env);
		if (id_str != NULL &&
			should_track_this_program(should_track_env)) {
			int shmem_id = atoi(id_str);
			state = (struct shared_basics *) shmat(shmem_id, NULL,
							       0);
			if (state == (void *) -1) {
				state = NULL;
			}
			if (custom_setup != NULL && custom_setup(state) < 0) {
				exit(-1);
			}
		}
		*init_ptr = 1;
	}
}
