#include "nondeterminism_ipc.h"
#include "ipc_parent.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char **alloc_argv(int argc, char **argv)
{
	char **new_argv = malloc(sizeof(char *) * (argc + 1));
	if (new_argv == NULL) {
		perror("Could not copy arguments");
		return NULL;
	} else {
		memcpy(new_argv, argv, sizeof(char *) * argc);
		new_argv[argc] = NULL;
		return new_argv;
	}
}

inline static volatile struct nondeterminism_state *
get_state_body(struct parent_state *state)
{
	return  (volatile struct nondeterminism_state *)
		get_shared_body(state->shared_state);
}

inline static void
reset_state(struct parent_state *state, unsigned recording_max)
{
	volatile struct nondeterminism_state *
	state_body = get_state_body(state);
	state_body->recording_max = recording_max;
	state_body->node_i = 0;
}

inline static unsigned get_n_nodes(struct parent_state *state)
{
	return get_state_body(state)->node_i;
}

static int init_nondeterminism(struct parent_state *state,
				const char *program_list,
				unsigned recording_max)
{
	int status;

	status = init_parent_state(state, NONDETERMINISM_ID_ENV,
					NONDETERMINISM_SHOULD_TRACK_ENV,
					program_list,
					sizeof(struct nondeterminism_state) +
						sizeof(void *) *
						recording_max);
	if (status < 0) {
		return -1;
	}
	return 0;
}

inline static int run_test(struct parent_state *state, char **new_argv,
				unsigned recording_max)
{
	reset_state(state, recording_max);
	return start_child(state, new_argv[0], new_argv, -1, -1, -1, NULL);
}

static long int preliminary(char **new_argv, const char *program_list)
{
	struct parent_state state;

	if (init_nondeterminism(&state, program_list, 0) < 0) {
		fprintf(stderr, "Could not initialize counting test.\n");
		return -1;
	} else {
		int status = run_test(&state, new_argv, 0);
		unsigned n_nodes = get_n_nodes(&state);
		destroy_parent_state(&state);
		return status < 0 ? status : n_nodes;
	}
}

inline static void **
get_nodes(struct parent_state *state)
{
	return (void **) get_state_body(state)->nodes;
}

static int
compare_to(struct parent_state *state, char **new_argv, unsigned recording_max,
		void **old_nodes, unsigned n_old_nodes)
{
	int result;
	result = run_test(state, new_argv, recording_max);
	if (result == 0) {
		void **new_nodes = get_nodes(state);
		unsigned n_new_nodes = get_n_nodes(state),
			min_n_nodes = n_new_nodes < n_old_nodes ?
					n_new_nodes : n_old_nodes;
		unsigned node_i;

		for (node_i = 1; node_i < min_n_nodes; node_i++) {
			void *old_node = old_nodes[node_i],
				*new_node = new_nodes[node_i];
			if (old_node != new_node) {
				void *previous_node = new_nodes[node_i - 1];
				printf("Diverged after %p: %p vs %p\n",
					previous_node, old_node, new_node);
				result = 1;
				break;
			}
		}
	}
	return result;
}

static int compare(char **new_argv, const char *program_list,
			unsigned recording_max)
{
	struct parent_state state;

	if (init_nondeterminism(&state, program_list, recording_max) < 0) {
		fprintf(stderr, "Could not initialize comparisons.\n");
		return -1;
	} else {
		int status = run_test(&state, new_argv, recording_max);
		if (status < 0) {
			fprintf(stderr,
				"Could not run child the first time.\n");
		} else {
			unsigned n_old_nodes = get_n_nodes(&state);
			size_t nodes_size = sizeof(void *) * n_old_nodes;
			void **old_nodes = malloc(nodes_size);

			if (old_nodes == NULL) {
				fprintf(stderr, "Could not allocate space for "
						"%u address nodes.\n",
					n_old_nodes);
				status = -1;
			} else {
				int result;
				memcpy(old_nodes, get_nodes(&state),
					nodes_size);
				while ((result = compare_to(&state, new_argv,
								recording_max,
								old_nodes,
								n_old_nodes))
					== 0) {
				}
				if (result < 0) {
					status = -1;
				}
				free(old_nodes);
			}
		}
		destroy_parent_state(&state);
		return status;
	}
}

static int track(int argc, char **argv, const char *program_list)
{
	char **new_argv = alloc_argv(argc, argv);
	if (new_argv == NULL) {
		return -1;
	} else {
		int status = 0;
		long int preliminary_status = preliminary(new_argv,
								program_list);
		if (preliminary_status < 0) {
			status = -1;
		} else {
			unsigned recording_max = (unsigned) preliminary_status;
			printf("Found %u nodes.\n", recording_max);
			status = compare(new_argv, program_list, recording_max);
		}

		free(new_argv);
		return status;
	}
}

#define OPTION_MARKER '-'
#define PROGRAM_LIST_FLAG_CHAR 'p'

static void print_usage(char *command)
{
	fprintf(stderr, "Usage: %s (%c%c optional program watch list) "
			"[test command] [test command arguments...]\n",
		command, OPTION_MARKER, PROGRAM_LIST_FLAG_CHAR);
}

static int maybe_get_optional(char **argv, int argc, char **program_list_ptr)
{
	char *program_list = NULL;
	int arg_i;

	*program_list_ptr = NULL;
	for (arg_i = 1; arg_i < argc; arg_i++) {
		char *current_arg = argv[arg_i];
		if (current_arg[0] == OPTION_MARKER) {
			char *option_value = argv[arg_i];
			arg_i++;
			switch (current_arg[1]) {
			case PROGRAM_LIST_FLAG_CHAR:
				*program_list_ptr = strdup(option_value);
			default:
				free(program_list);
				return -1;
			}
		} else {
			break;
		}
	}
	*program_list_ptr = program_list;
	return arg_i;
}

int main(int argc, char *argv[])
{
	char *program_list;
	int command_start;

	command_start = maybe_get_optional(argv, argc, &program_list);
	if (command_start >= argc) {
		print_usage(argv[0]);
		return -1;
	}

	return track(argc - command_start, argv + command_start, program_list);
}
