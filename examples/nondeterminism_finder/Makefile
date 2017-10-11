.PHONY: instrumentation samples

LIB_DIR=../../
CFLAGS+=-Wall -Werror

ORIGINAL_TARGETS=instrumentation nondeterminism-tracker
SAMPLES_DIR=samples
DIR_TARGETS=$(SAMPLES_DIR)
TARGETS=$(ORIGINAL_TARGETS) $(DIR_TARGETS)
all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ -I$(LIB_DIR) $<
nondeterminism-tracker: nondeterminism-tracker.o
	$(CC) $(LDFLAGS) -o $@ $< $(LIB_DIR)ipc_parent.o

instrumentation:
	bash ../../build_flex.bash $(abspath nondeterminism-) \
		$(abspath nondeterminism_instrumenter.cc) \
		$(abspath nondeterminism_instrumenter.so) \
		$(abspath nondeterminism_rt.c) \
		$(abspath nondeterminism_rt.o)

$(SAMPLES_DIR): nondeterminism_instrumenter.so nondeterminism_rt.o
	make -C $(SAMPLES_DIR) clean all

clean:
	rm -rf flex_basic.o nondeterminism-clang nondeterminism-clang.o \
		nondeterminism-clang++ nondeterminism_instrumenter.so \
		nondeterminism-ld nondeterminism-ld.o nondeterminism_rt.o \
		nondeterminism-tracker.o nondeterminism-tracker
	make -C samples clean
