OBJ_NAMES=ipc_parent ipc_child
OBJS=$(OBJ_NAMES:%=%.o)

CFLAGS+=-Wall -Werror

all: $(OBJS)

clean:
	rm -rf $(OBJS)
