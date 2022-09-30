#if defined(_WIN32)
#if defined(_DEBUG) && !defined(WINRT_NATVIS)
#define WINRT_NATVIS
#endif
#include <winrt/base.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>

import std.filesystem;
//import boost.ut;

import gles;

namespace fs = std::filesystem;

__declspec(dllimport) void test_egl_resume(EGLDisplay display, EGLSurface es_surfae);

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    test_egl_resume(eglGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_SURFACE);
    return 0;
}
