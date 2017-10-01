COMPILER_BASE=$(realpath ../)/nondeterminism-
CC=$(COMPILER_BASE)clang
CXX=$(COMPILER_BASE)clang++
LD=$(COMPILER_BASE)ld

BINS=use_random
OBJS=$(BINS:%=%.o)
TARGETS=$(OBJS) $(BINS)

all: $(TARGETS)

%: %.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
