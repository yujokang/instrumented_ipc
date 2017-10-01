#ifndef IPC_BASIC_H
#define IPC_BASIC_H
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>

struct shared_basics {
	pid_t current_process;
	void *shared_alignment[0];
};

inline static volatile void *
get_shared_body(volatile struct shared_basics *basics)
{
	return &basics->shared_alignment[0];
}

#define PROGRAM_LIST_DELIM ','

#endif /* IPC_BASIC_H */
