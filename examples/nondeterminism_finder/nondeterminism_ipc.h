#ifndef NONDETERMINISM_IPC_H
#define NONDETERMINISM_IPC_H

#define NONDETERMINISM_ID_ENV "NONDETERMINISM_SHMEM_ID"
#define NONDETERMINISM_SHOULD_TRACK_ENV "NONDETERMINISM_SHOULD_TRACK"

struct nondeterminism_state {
	unsigned recording_max;
	unsigned node_i;
	void *nodes[0];
};

#endif /* NONDETERMINISM_IPC_H */
