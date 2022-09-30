module;
// There was unresolved symbol WINRT_abi_val, WINRT_get_val
#if defined(_WIN32)
#if defined(_DEBUG) && !defined(WINRT_NATVIS)
#define WINRT_NATVIS
#endif
#include <winrt/base.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <spdlog/spdlog.h>

// import std.filesystem;
import std.memory;
import std.threading;

module gles;

__declspec(dllexport) void test_egl_resume(EGLDisplay display, EGLSurface es_surfae) {
    egl_context_t context{display, EGL_NO_CONTEXT};
    egl_surface_owner_t surface{display, context.config(), es_surfae};

    if (int ec = context.resume(surface.handle(), EGL_NO_CONFIG_KHR); ec != 0)
        throw std::system_error{ec, get_opengl_category(), "egl_context_t::resume"};

    GLint versions[2]{};
    if (get_opengl_version(versions[0], versions[1]) == false)
        spdlog::warn("{}: {} failed", __func__, "get_opengl_version");
    else
        spdlog::info("{}: OpenGL ES {}.{}", __func__, versions[0], versions[1]);

    if (int ec = context.suspend(); ec != 0)
        throw std::system_error{ec, get_opengl_category(), "egl_context_t::suspend"};
}

//#pragma comment(linker, "/include:WINRT_abi_val")
//#pragma comment(linker, "/include:WINRT_get_val")

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
