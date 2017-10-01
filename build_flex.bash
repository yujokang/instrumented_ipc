#!/bin/bash

# Get command line arguments
if [ "$#" -ne 5 ]; then
	echo Usage $0 [compiler chain path base] [input instrumenter source] \
			[output instrumenter shared object] \
			[runtime object input source] [runtime output object] \
			1>&2
	exit -1
fi

compiler_chain_path_base=$1
instrumenter_src=$2
instrumenter_so=$(realpath $3)
rt_src=$4
rt_o=$(realpath $5)

# Basic path
script_dir=$(realpath $(dirname $0))

# Build instrumenter
if [ -z $LLVM_CONFIG ]; then
	LLVM_CONFIG=llvm-config
fi
if [ -z $CLANG ]; then
	CLANG=clang
fi

LLVM_CFLAGS=$($LLVM_CONFIG --cxxflags)
LLVM_LDFLAGS=$($LLVM_CONFIG --ldflags)
CFLAGS+=" -Wall -Werror"
CXXFLAGS+=

instrumenter_o=${instrumenter_src}.o
$CLANG $LLVM_CFLAGS $CXXFLAGS -fPIC -c $instrumenter_src -o $instrumenter_o
${CLANG}++ $LLVM_CFLAGS -shared $CXXFLAGS $instrumenter_o $LLVM_LDFLAGS \
	$LDFLAGS -o $instrumenter_so
rm -f $instrumenter_o

# Choose compiler, or default
if [ -z $CC ]; then
	CC=clang
fi
# Build runtime library
$CC $CFLAGS -I$script_dir -c -o $rt_o $rt_src

# Build compilers and linkers
clang_command=$(which clang)
ld_command=$(which ld)
basic_base=flex_basic.
basic_src=$script_dir/${basic_base}c
basic_o=${basic_base}o
child_o=$script_dir/ipc_child.o
cc_bin=${compiler_chain_path_base}clang
cc_o=${cc_bin}.o
cxx_bin=${cc_bin}++
ld_bin=${compiler_chain_path_base}ld
ld_o=${ld_bin}.o

$CC $CFLAGS -DRT_OBJ=$rt_o -DCHILD_OBJ=$child_o -c -o $basic_o \
	$basic_src
$CC $CFLAGS -c -o $cc_o -DEXTRA_SO_VALUE=$instrumenter_so -DBUILD_DEBUG \
	-DBARE_COMMAND=`which clang` $script_dir/clang_flex.c
$CC $LDFLAGS -o $cc_bin $cc_o $basic_o
if [ ! -L $cxx_bin ]; then
	ln -s $cc_bin $cxx_bin
fi
$CC $CFLAGS -c -o $ld_o -DBARE_COMMAND=`which ld` $script_dir/ld_flex.c
$CC $LDFLAGS -o $ld_bin $ld_o $basic_o
