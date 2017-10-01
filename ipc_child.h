#ifndef IPC_CHILD_H
#define IPC_CHILD_H

#include "ipc_basic.h"

void init_child(volatile struct shared_basics *state, int *init_ptr,
		const char *id_env, const char *should_track_env,
		int (*custom_setup)(volatile struct shared_basics *state));
#define CREATE_SETUP(init_name, state_var, init_var, id_env, should_track_env, \
			custom_setup) \
static volatile struct shared_basics state_var; \
static int init_var = 0; \
__attribute__((constructor)) void init_name(void) \
{ \
	init_child(&state_var, &init_var, id_env, should_track_env, \
			custom_setup); \
}
#endif /* IPC_CHILD_H */
