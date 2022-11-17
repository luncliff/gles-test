#!/bin/bash

CC=g++
TARGET=testtree
SOURCE=(
	testtree.cpp
)
CFLAGS=(
	-pipe
	-fno-exceptions
	-fno-rtti
	-ffast-math
	-fstrict-aliasing
	-DTREE_NUM_THREADS=1
)
LFLAGS=(
	-lstdc++
	-lrt
	-lpthread
)

if [[ $HOSTTYPE == "arm" ]]; then

	UNAME_SUFFIX=`uname -r | grep -o -E -e -[^-]+$`

	if [[ $UNAME_SUFFIX == "-efikamx" ]]; then

		CFLAGS+=(
			-marm
			-march=armv7-a
			-mtune=cortex-a8
			-mcpu=cortex-a8
		)
	fi

elif [[ $HOSTTYPE == "x86_64" ]]; then

	CFLAGS+=(
# Set -march and -mtune accordingly:
#		-march=btver1
#		-mtune=btver1
	)

elif [[ $HOSTTYPE == "powerpc" ]]; then

	UNAME_SUFFIX=`uname -r | grep -o -E -e -[^-]+$`

	if [[ $UNAME_SUFFIX == "-wii" ]]; then

		CFLAGS+=(
			-mpowerpc
			-mcpu=750
		)

	elif [[ $UNAME_SUFFIX == "-powerpc" || $UNAME_SUFFIX == "-smp" ]]; then

		CFLAGS+=(
			-mpowerpc
			-mcpu=7450
		)
	fi

elif [[ $HOSTTYPE == "powerpc64" || $HOSTTYPE == "ppc64" ]]; then

	CFLAGS+=(
		-mpowerpc64
		-mcpu=powerpc64
		-mtune=power6
	)
fi

if [[ $1 == "debug" ]]; then
	CFLAGS+=(
		-Wall
		-O0
		-g
		-DDEBUG)
else
	CFLAGS+=(
		-funroll-loops
		-O3
		-DNDEBUG)
fi

BUILD_CMD=$CC" -o "$TARGET" "${CFLAGS[@]}" "${SOURCE[@]}" "${LFLAGS[@]}
echo $BUILD_CMD
$BUILD_CMD
