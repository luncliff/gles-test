#!/bin/bash

TARGET=test_unknown_linear
SOURCE=(
	app_linear.cpp
	utilPix.cpp
	utilTex.cpp
	get_file_size.cpp
)
CFLAGS=(
	-pipe
	-fno-exceptions
	-fno-rtti
	-ffast-math
	-fstrict-aliasing
	-Wtrigraphs
	-Wreturn-type
	-Wunused-variable
	-Wunused-value
)
LFLAGS=(
	-lstdc++
	-lrt
	-ldl
)

# you may need to:
# sudo apt-get install libxrandr-dev
#
#SOURCE+=(xrandr_util.cpp)
#CFLAGS+=(-DUSE_XRANDR)
#LFLAGS+=(-lXrandr)

if [[ $HOSTTYPE == "arm" ]]; then

	UNAME_SUFFIX=`uname -r | grep -o -E -e -[^-]+$`

	if [[ $UNAME_SUFFIX == "-efikamx" ]]; then

		SOURCE+=(
			main.cpp
			amd_perf_monitor.cpp
		)
		CFLAGS+=(
			-marm
			-mcpu=cortex-a8
			-mfpu=neon
			-D_LINUX
			-DEGL_EGLEXT_PROTOTYPES
			-DGL_GLEXT_PROTOTYPES
		)
		LFLAGS+=(
			-lGLESv2
			-lEGL
			-lX11
		)
	fi

elif [[ ${HOSTTYPE:0:3} == "x86" ]]; then

	SOURCE+=(
		main_glx.cpp
		amd_perf_monitor.cpp
	)
	CFLAGS+=(
		-msse3
		-mfpmath=sse
		-DPLATFORM_GLX
		-DGLX_GLXEXT_PROTOTYPES
		-DGLCOREARB_PROTOTYPES
		-DGL_GLEXT_PROTOTYPES
	)
	LFLAGS+=(
		-lGL
		-lX11
	)
fi

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

BUILD_CMD="g++ -o "$TARGET" "${CFLAGS[@]}" "${SOURCE[@]}" "${LFLAGS[@]}
echo $BUILD_CMD
$BUILD_CMD
