#include <ipc_child.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "nondeterminism_ipc.h"

volatile struct nondeterminism_state *state_body = NULL;
static char *ignored = NULL;

static int __nondeterminism_extra_setup(volatile struct shared_basics *state)
{
	char *ignored_env;
	state_body = (volatile struct nondeterminism_state *)
			get_shared_body(state);
	ignored_env = getenv(NONDETERMINISM_IGNORE_ENV);
	if (ignored_env != NULL) {
		ignored = strdup(ignored_env);
		if (ignored == NULL) {
			abort();
		}
	}
	return 0;
}

CREATE_SETUP(__init_nondeterminism, shared_state, initialized,
		NONDETERMINISM_ID_ENV, NONDETERMINISM_SHOULD_TRACK_ENV,
		__nondeterminism_extra_setup)

static int should_ignore_file(const char *name)
{
	if (ignored == NULL) {
		return 0;
	}

	return find_in_list(name, ignored);
}

void __nondeterminism_record_node(const char *name)
{
	unsigned current_node_i;

	if (state_body == NULL || should_ignore_file(name)) {
		return;
	}
	shared_state.current_process = getpid();

	current_node_i = state_body->node_i;
	if (current_node_i < state_body->recording_max) {
		state_body->nodes[current_node_i] = __builtin_return_address(0);
	}
	state_body->node_i = current_node_i + 1;
}
