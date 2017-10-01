#include "ipc_parent.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define INT_MAX_LEN 12

static int export_shmem(const char *id_env, int id)
{
	char id_buf[INT_MAX_LEN];

	if (snprintf(id_buf, sizeof(id_buf), "%d", id) < 0) {
		fprintf(stderr, "Could not write shared memory ID, %d: ", id);
		perror(NULL);
		return -1;
	}

	if (setenv(id_env, id_buf, 1) < 0) {
		perror("Could not export shared memory ID");
		return -1;
	}

	return 0;
}

inline static int cleanup_shmem(int id)
{
	if (id >= 0 && shmctl(id, IPC_RMID, NULL) < 0) {
		perror("Failed to destroy shared memory");
		return -1;
	}
	return 0;
}

static struct shared_basics *
alloc_shared_basics(const char *id_env, size_t body_size, int *id_ptr)
{
	size_t shm_size = sizeof(struct shared_basics) + body_size;
	int id;

	*id_ptr = 0;

	id = shmget(IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR |
						S_IWUSR);
	if (id < 0) {
		perror("Could not get a shared memory ID");
		return NULL;
	} else {
		struct shared_basics *alloced_shmem = shmat(id, NULL, 0);
		if (alloced_shmem == NULL) {
			fprintf(stderr,
				"Could not get shared memory using ID, %d: ",
				id);
			perror(NULL);
		} else if (export_shmem(id_env, id) == 0) {
			*id_ptr = id;
			return alloced_shmem;
		}
		cleanup_shmem(id);
		return NULL;
	}
}

static const struct itimerval no_timeout = {
	.it_value = {
		.tv_sec = 0,
		.tv_usec = 0
	}
};
#define NO_TIMEOUT (&no_timeout)

inline static void reset_timeout(struct parent_state *to_reset)
{
	memcpy(&to_reset->timeout, NO_TIMEOUT,
		sizeof(to_reset->timeout));
}

static int alloc_tracking_strings(const char *should_track_env,
					const char *program_list)
{
	return 0;
}

inline static int cleanup_state_shmem(struct parent_state *to_cleanup)
{
	int status = 0;
	if (cleanup_shmem(to_cleanup->shmem_id) < 0) {
		status = -1;
	} else {
		to_cleanup->shmem_id = -1;
	}
	to_cleanup->shared_state = NULL;

	return status;
}

int init_parent_state(struct parent_state *to_init, const char *id_env,
			const char *should_track_env,
			const char *program_list, size_t body_size)
{
	to_init->shared_state = alloc_shared_basics(id_env, body_size,
							&to_init->shmem_id);
	if (to_init->shared_state == NULL) {
		return -1;
	}

	if (alloc_tracking_strings(should_track_env, program_list)) {
		cleanup_state_shmem(to_init);
		return -1;
	}

	to_init->null_in_fd = -1;
	to_init->null_out_fd = -1;
	reset_timeout(to_init);

	return 0;
}

#define GEN_CLOSE_FD_FUNC(direction, direction_str) \
inline static int destroy_##direction##_fd(struct parent_state *state) \
{ \
	if (state->null_##direction##_fd >= 0) { \
		if (close(state->null_##direction##_fd) < 0) { \
			perror("Failed to close dummy " direction_str \
				" file"); \
			return -1; \
		} \
		state->null_##direction##_fd = 0; \
	} \
	return 0; \
}

GEN_CLOSE_FD_FUNC(out, "output")
GEN_CLOSE_FD_FUNC(in, "input")

int destroy_parent_state(struct parent_state *to_destroy)
{
	int status = 0;

	if (cleanup_state_shmem(to_destroy) < 0) {
		status = -1;
	}

	if (destroy_in_fd(to_destroy) < 0) {
		status = -1;
	}
	if (destroy_out_fd(to_destroy) < 0) {
		status = -1;
	}

	reset_timeout(to_destroy);

	return status;
}

void divert(int to_divert_to, int to_divert_from, const char *stream_name)
{
	if (dup2(to_divert_to, to_divert_from) < 0) {
		fprintf(stderr, "Failed to redirect %s to %d: ", stream_name,
			to_divert_to);
		perror("");
		exit(-1);
	}
}

static pid_t immediate_child = -1, *running_descendant_ptr = NULL;
static int need_timeout = 1;

static void handle_child_timeout(int signal)
{
	if (immediate_child > 0) {
		if (kill(immediate_child, SIGKILL) < 0 && errno != ESRCH) {
			fprintf(stderr, "Failed to kill child %d: ",
				immediate_child);
			perror("");
		}
		if (running_descendant_ptr != NULL &&
			*running_descendant_ptr > 0 &&
			*running_descendant_ptr != immediate_child &&
			kill(*running_descendant_ptr, SIGKILL) < 0 &&
				errno != ESRCH) {
			fprintf(stderr, "Failed to kill running child %d: ",
				*running_descendant_ptr);
			perror("");
		}
		immediate_child = -1;
	}
}

static int _start_child(struct parent_state *state, char *child_command,
			char **child_argv, int real_in_fd, int real_out_fd,
			int real_err_fd, int *status_ptr)
{
	pid_t child;

	running_descendant_ptr = &state->shared_state->current_process;
	*running_descendant_ptr = -1;
	if (need_timeout && signal(SIGALRM, handle_child_timeout) == SIG_ERR) {
		perror("Could not create timeout handler");
		return -1;
	}
	need_timeout = 0;

	child = fork();
	if (child < 0) {
		perror("Failed to start target");
		return -1;
	} else if (child > 0) {
		int status;

		immediate_child = child;
		if (setitimer(ITIMER_REAL, &state->timeout, NULL)) {
			perror("Failed to start timeout");
			return -1;
		}
		if (waitpid(child, &status, 0) < 0) {
			perror("Failed to wait for child");
			return -1;
		}
		if (setitimer(ITIMER_REAL, NO_TIMEOUT, NULL)) {
			perror("Failed to stop timeout");
			return -1;
		}
		if (status_ptr != NULL) {
			*status_ptr = WEXITSTATUS(status);
		}
		return 0;
	} else {
		divert(real_in_fd, STDIN_FILENO, "stdin");
		divert(real_out_fd, STDOUT_FILENO, "stdout");
		divert(real_err_fd, STDERR_FILENO, "stderr");
		exit(execv(child_command, child_argv));
	}

	return 0;
}

#define NULL_PATH "/dev/null"

int start_child(struct parent_state *state,
		char *child_command, char **child_argv, int in_fd, int out_fd,
		int err_fd, int *status_ptr)
{
	int status = 0;

	if (in_fd < 0 && state->null_in_fd < 0) {
		state->null_in_fd = open(NULL_PATH, O_RDONLY);
		if (state->null_in_fd < 0) {
			perror("Failed to open dummy input file");
			return -1;
		}
	}

	if ((out_fd < 0 || err_fd < 0) && state->null_out_fd < 0) {
		state->null_out_fd = open(NULL_PATH, O_WRONLY);
		if (state->null_out_fd < 0) {
			perror("Failed to open dummy output file");
			status = -1;
		}
	}

	if (status == 0) {
		status = _start_child(state, child_command, child_argv,
					in_fd < 0 ? state->null_in_fd : in_fd,
					out_fd < 0 ? state->null_out_fd :
							out_fd,
					err_fd < 0 ? state->null_out_fd :
							out_fd, status_ptr);
	}

	if (status < 0) {
		destroy_in_fd(state);
	}
	return status;
}
