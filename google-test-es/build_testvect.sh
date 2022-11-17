#!/bin/bash

CC=g++
TARGET=testvect
SOURCE=(
	testvect.cpp
)
CFLAGS=(
	-pipe
	-fno-exceptions
	-fno-rtti
	-ffast-math
	-fstrict-aliasing
	-ftree-vectorizer-verbose=2
)
LFLAGS=(
	-lstdc++
	-lrt
)

if [[ $HOSTTYPE == "arm" ]]; then

	UNAME_SUFFIX=`uname -r | grep -o -E -e -[^-]+$`

	if [[ $UNAME_SUFFIX == "-efikamx" ]]; then

		CFLAGS+=(
			-marm
			-mcpu=cortex-a8
			-mfpu=neon
		)
	fi

elif [[ ${HOSTTYPE:0:3} == "x86" ]]; then

	CFLAGS+=(
		-msse3
		-mfpmath=sse
	)

elif [[ $HOSTTYPE == "powerpc" ]]; then

	UNAME_SUFFIX=`uname -r | grep -o -E -e -[^-]+$`

	if [[ $UNAME_SUFFIX == "-wii" ]]; then

		CFLAGS+=(
			-mpowerpc
			-mcpu=750
			-mpaired
		)
	fi
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
