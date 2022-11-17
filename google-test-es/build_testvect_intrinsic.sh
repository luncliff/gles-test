#!/bin/bash

CC=g++
TARGET=testvect_intrinsic
SOURCE=(
	testvect_intrinsic.cpp
)
CFLAGS=(
	-pipe
	-fno-exceptions
	-fno-rtti
	-ffast-math
	-fstrict-aliasing
	-DSIMD_NUM_THREADS=2
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
			-mfloat-abi=softfp
			-marm
			-march=armv7-a
			-mtune=cortex-a8
			-mcpu=cortex-a8
			-mfpu=neon
			-DSIMD_AUTOVECT=SIMD_4WAY
		)
	fi

elif [[ $HOSTTYPE == "x86_64" ]]; then

	CFLAGS+=(
# Set -march and -mtune accordingly:
#		-march=btver1
#		-mtune=btver1
# For SSE, establish a baseline:
		-msse3
		-mfpmath=sse
# Use xmm intrinsics with SSE (rather than autovectorization) to level the
# ground with altivec
#		-DSIMD_AUTOVECT=SIMD_4WAY
# For AVX, disable any SSE which might be enabled by default:
#		-mno-sse
#		-mno-sse2
#		-mno-sse3
#		-mno-ssse3
#		-mno-sse4
#		-mno-sse4.1
#		-mno-sse4.2
#		-mavx
#		-DSIMD_AUTOVECT=SIMD_4WAY
# For AVX with FMA4 (cumulative to above):
#		-mfma4
#		-ffp-contract=fast
	)

elif [[ $HOSTTYPE == "powerpc" ]]; then

	UNAME_SUFFIX=`uname -r | grep -o -E -e -[^-]+$`

	if [[ $UNAME_SUFFIX == "-wii" ]]; then

		CFLAGS+=(
			-mpowerpc
			-mcpu=750
			-mpaired
			-DSIMD_AUTOVECT=SIMD_2WAY
		)

	elif [[ $UNAME_SUFFIX == "-powerpc" || $UNAME_SUFFIX == "-smp" ]]; then

		CFLAGS+=(
			-mpowerpc
			-mcpu=7450
			-maltivec
			-mvrsave
# Don't use autovectorization with altivec - as per 4.6.3 gcc will not use
# splat/permute altivec ops during autovectorization, rendering that useless.
#			-DSIMD_AUTOVECT=SIMD_4WAY
		)
	fi

elif [[ $HOSTTYPE == "powerpc64" || $HOSTTYPE == "ppc64" ]]; then

	CFLAGS+=(
		-mpowerpc64
		-mcpu=powerpc64
		-mtune=power6
		-maltivec
		-mvrsave
# Don't use autovectorization with altivec - as per 4.6.3 gcc will not use
# splat/permute altivec ops during autovectorization, rendering that useless.
#		-DSIMD_AUTOVECT=SIMD_4WAY
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
