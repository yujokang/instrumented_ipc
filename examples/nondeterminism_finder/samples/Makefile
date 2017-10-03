COMPILER_BASE=$(realpath ../)/nondeterminism-
CC=$(COMPILER_BASE)clang
CXX=$(COMPILER_BASE)clang++
LD=$(COMPILER_BASE)ld

BINS=use_randomized deterministic
OBJ_NAMES=randomized $(BINS)
OBJS=$(OBJ_NAMES:%=%.o)
TARGETS=$(OBJS) $(BINS)

all: $(TARGETS)

use_randomized: use_randomized.o randomized.o
	$(CC) $(LDFLAGS) -o $@ $^
%: %.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)
