module;
#include <string>
#include <system_error>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

#if defined(_WIN32)
#include <d3d11.h>
#include <winrt/base.h>
#endif

export module gles;

#include "gles.hpp"
