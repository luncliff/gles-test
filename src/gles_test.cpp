#if defined(_WIN32)
#if defined(_DEBUG) && !defined(WINRT_NATVIS)
#define WINRT_NATVIS
#endif
#include <winrt/base.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <filesystem>

//import std.filesystem;
//import boost.ut;

import gles;

namespace fs = std::filesystem;

bool test_egl_resume(EGLDisplay es_display) noexcept;

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    try {
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        test_egl_resume(display);
        eglTerminate(display);
    } catch (...) {
        return 1;
    }
    return 0;
}
