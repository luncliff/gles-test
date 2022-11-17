#!/bin/bash

# you may need to:
#
# sudo apt-get install libxrandr-dev

TARGET=test_imx5_tex_yuv
SOURCE=(
	main.cpp
	app_tex_yuv.cpp
	utilPix.cpp
	utilTex.cpp
	get_file_size.cpp
	amd_perf_monitor.cpp
	xrandr_util.cpp
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
	-DUSE_XRANDR
	-D_LINUX
	-DEGL_EGLEXT_PROTOTYPES
	-DGL_GLEXT_PROTOTYPES
)
LFLAGS=(
	-lstdc++
	-ldl
	-lrt
	-lGLESv2
	-lEGL
	-lX11
	-lXrandr
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
