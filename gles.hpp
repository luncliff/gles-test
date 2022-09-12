#pragma once
#include <string>
#include <system_error>
#include <vector>

#define EGL_EGL_PROTOTYPES 1
// #define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#define GL_GLES_PROTOTYPES 1
// #define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>

bool check_extensions(std::vector<std::string>& output) noexcept;

class egl_surface_owner_t final {
    EGLDisplay display;
    EGLConfig config;
    EGLSurface surface = EGL_NO_SURFACE;

  public:
    egl_surface_owner_t(EGLDisplay display, EGLConfig config, EGLSurface surface) noexcept;
    ~egl_surface_owner_t() noexcept;
    egl_surface_owner_t(egl_surface_owner_t const&) = delete;
    egl_surface_owner_t& operator=(egl_surface_owner_t const&) = delete;
    egl_surface_owner_t(egl_surface_owner_t&&) = delete;
    egl_surface_owner_t& operator=(egl_surface_owner_t&&) = delete;

    EGLint get_size(EGLint& width, EGLint& height) noexcept;
    EGLSurface handle() const noexcept;
};

class egl_context_t final {
  private:
    EGLDisplay display;
    EGLint versions[2]{};   // major, minor
    EGLConfig configs[1]{}; // ES 2.0, Window/Pbuffer, RGBA 32, Depth 16
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

  public:
    egl_context_t(EGLDisplay display, EGLContext share_context) noexcept;
    ~egl_context_t() noexcept;
    egl_context_t(egl_context_t const&) = delete;
    egl_context_t& operator=(egl_context_t const&) = delete;
    egl_context_t(egl_context_t&&) = delete;
    egl_context_t& operator=(egl_context_t&&) = delete;

    void destroy() noexcept;
    EGLint resume(EGLSurface es_surface, EGLConfig) noexcept;
    EGLint suspend() noexcept;
    EGLint swap() noexcept;

    EGLContext handle() const noexcept;
    EGLConfig config() const noexcept;
};

bool get_opengl_version(GLint& major, GLint& minor) noexcept;

std::error_category& get_opengl_category() noexcept;
