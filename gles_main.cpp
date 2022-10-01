module;
#include <future>
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

/// @see https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#if defined(_WIN32)
#define _INTERFACE_ __declspec(dllexport)
#else
#define _INTERFACE_
#endif

export module gles;

#include "gles.hpp"
