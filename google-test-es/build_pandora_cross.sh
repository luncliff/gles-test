#!/bin/bash

# pandora cross-compile params courtesy of Paeryn from the gp32x.com forums

TARGET=test_pandora_sphere
SOURCE=(
	main.cpp
	app_sphere.cpp
	utilPix.cpp
	utilTex.cpp
	get_file_size.cpp
	amd_perf_monitor.cpp
)
CFLAGS=(
	-pipe
	-marm
	-mcpu=cortex-a8
	-mfpu=neon
	-fno-exceptions
	-fno-rtti
	-ffast-math
	-fstrict-aliasing
	-DSUPPORT_X11
)
LFLAGS=(
	-lstdc++
	-ldl
	-lrt
	-lGLESv2
	-lEGL
	-lIMGegl
	-lsrv_um
	-lX11
	-lXdmcp
	-lXau
)

if [[ $1 == "debug" ]]; then
	CFLAGS+=(
		-Wall
		-O0
		-g
		-DDEBUG
	)
else
	CFLAGS+=(
		-funroll-loops
		-O3
		-DNDEBUG
	)
fi

arm-none-linux-gnueabi-gcc -o $TARGET ${CFLAGS[@]} ${SOURCE[@]} ${LFLAGS[@]}
