#pragma once
#include <system_error>

#define EGL_EGL_PROTOTYPES 1
// #define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#define GL_GLES_PROTOTYPES 1
// #define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
