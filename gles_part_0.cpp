module;
// There was unresolved symbol WINRT_abi_val, WINRT_get_val
#if defined(_WIN32)
#if defined(_DEBUG) && !defined(WINRT_NATVIS)
//#pragma comment(linker, "/include:WINRT_abi_val")
//#pragma comment(linker, "/include:WINRT_get_val")
#define WINRT_NATVIS
#endif
#include <winrt/base.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <filesystem>
#include <spdlog/spdlog.h>

// CI environment doesn't support this yet
// import std.filesystem;
// import std.memory;
// import std.threading;

/// @see https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
#if defined(_WIN32)
#define _INTERFACE_ __declspec(dllexport)
#else
#define _INTERFACE_
#endif

module gles;

egl_context_guard::egl_context_guard(egl_context_t& context, EGLSurface surface) noexcept(false) : context{context} {
    if (int ec = context.resume(surface, EGL_NO_CONFIG_KHR); ec != 0)
        throw std::system_error{ec, get_opengl_category(), "egl_context_t::resume"};
}

egl_context_guard::~egl_context_guard() noexcept(false) {
    if (int ec = context.suspend(); ec != 0)
        throw std::system_error{ec, get_opengl_category(), "egl_context_t::suspend"};
}

_INTERFACE_ bool test_egl_resume(EGLDisplay es_display) noexcept {
    egl_context_t context{es_display, EGL_NO_CONTEXT};
    EGLint attrs[]{EGL_WIDTH, 128, EGL_HEIGHT, 128, EGL_NONE};
    egl_surface_owner_t surface{es_display, context.config(),
                                eglCreatePbufferSurface(es_display, context.config(), attrs)};
    egl_context_guard guard{context, surface.handle()};

    GLint versions[2]{};
    if (get_opengl_version(versions[0], versions[1]) == false)
        return false;

    spdlog::info("{}: OpenGL ES {}.{}", __func__, versions[0], versions[1]);
    return true;
}

#if defined(_WIN32)

BOOL APIENTRY DllMain(HANDLE handle, DWORD reason, LPVOID) {
    static HMODULE g_module = NULL;
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_module = static_cast<HMODULE>(handle);
        break;
    default:
        break;
    }
    return TRUE;
}

#endif
