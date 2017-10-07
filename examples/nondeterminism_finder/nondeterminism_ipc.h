#ifndef NONDETERMINISM_IPC_H
#define NONDETERMINISM_IPC_H

#define NONDETERMINISM_ID_ENV "NONDETERMINISM_SHMEM_ID"
#define NONDETERMINISM_SHOULD_TRACK_ENV "NONDETERMINISM_SHOULD_TRACK"
#define NONDETERMINISM_IGNORE_ENV "NONDETERMINISM_IGNORE"

struct run_node {
	void *location;
	const char *name;
};

struct nondeterminism_state {
	unsigned recording_max;
	unsigned node_i;
	struct run_node nodes[0];
};

#endif /* NONDETERMINISM_IPC_H */
