#ifndef IPC_PARENT_H
#define IPC_PARENT_H

#include "ipc_basic.h"

#include <sys/time.h>

struct parent_state {
	int shmem_id;
	struct shared_basics *shared_state;
	int null_in_fd, null_out_fd;
	struct itimerval timeout;
	char *should_track_env;
	char *program_list;
};

int init_parent_state(struct parent_state *to_init, const char *id_env,
			const char *should_track_env,
			const char *program_list, size_t body_size);
int destroy_parent_state(struct parent_state *to_destroy);
int start_child(struct parent_state *state,
		char *child_command, char **child_argv, int in_fd, int out_fd,
		int err_fd, int *status_ptr);

#endif /* IPC_PARENT_H */
