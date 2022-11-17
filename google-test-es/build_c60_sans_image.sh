#!/bin/bash

TARGET=test_c60_sans_image
SOURCE=(
	main_glx.cpp
	app_sans_image.cpp
	get_file_size.cpp
	amd_perf_monitor.cpp
)
CFLAGS=(
	-pipe
	-msse3
	-mfpmath=sse
	-fno-exceptions
	-fno-rtti
	-ffast-math
	-fstrict-aliasing
	-DPLATFORM_GLX
	-DGLX_GLXEXT_PROTOTYPES
	-DGLCOREARB_PROTOTYPES
	-DGL_GLEXT_PROTOTYPES
)
LFLAGS=(
	-lstdc++
	-ldl
	-lrt
	-lGL
	-lX11
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

g++ -o $TARGET ${CFLAGS[@]} ${SOURCE[@]} ${LFLAGS[@]}
