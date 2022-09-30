module;
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <vector>

#define EGL_EGL_PROTOTYPES 1 // #define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#define GL_GLES_PROTOTYPES 1 // #define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>

module gles;

class tex2d_owner_t final {
    GLuint tex{};

  public:
    tex2d_owner_t(GLsizei w, GLsizei h) noexcept(false);
    ~tex2d_owner_t() noexcept;
    GLuint handle() const noexcept;

    GLenum update(GLsizei w, GLsizei h, const void* ptr) noexcept(false);
};

class fbo_owner_t final {
    GLuint fbo{};

  public:
    fbo_owner_t(GLuint tex) noexcept;
    ~fbo_owner_t() noexcept;

    GLuint handle() const noexcept;
};

using reader_callback_t = void (*)(void* user_data, const void* mapping, size_t length);

class pbo_reader_t final {
    static constexpr uint16_t capacity = 2;

  private:
    GLuint pbos[capacity];
    uint32_t length; // byte length of the buffer modification
    GLintptr offset;
    GLint version;

  public:
    explicit pbo_reader_t(GLuint length) noexcept(false);
    ~pbo_reader_t() noexcept;

    uint32_t get_length() const noexcept;
    GLenum pack(uint16_t idx, GLuint fbo, const GLint frame[4], //
                GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE) noexcept;
    GLenum map_and_invoke(uint16_t idx, reader_callback_t callback, void* user_data) noexcept;
};

using writer_callback_t = void (*)(void* user_data, void* mapping, size_t length);

class pbo_writer_t final {
    static constexpr uint16_t capacity = 2;

  private:
    GLuint pbos[capacity];
    uint32_t length;
    GLint version;

  public:
    explicit pbo_writer_t(GLuint length) noexcept(false);
    ~pbo_writer_t() noexcept;
    pbo_writer_t(pbo_writer_t const&) = delete;
    pbo_writer_t& operator=(pbo_writer_t const&) = delete;
    pbo_writer_t(pbo_writer_t&&) = delete;
    pbo_writer_t& operator=(pbo_writer_t&&) = delete;

    uint32_t get_length() const noexcept;
    GLenum unpack(uint16_t idx, GLuint tex2d, const GLint frame[4], //
                  GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE) noexcept;
    GLenum map_and_invoke(uint16_t idx, writer_callback_t callback, void* user_data) noexcept;
};

EGLint get_configs(EGLDisplay display, EGLConfig* configs, EGLint& count, const EGLint* attrs) noexcept {
    constexpr auto color_size = 8;
    constexpr auto depth_size = 16;
    EGLint backup_attrs[]{EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
                          EGL_BLUE_SIZE,       color_size,         EGL_GREEN_SIZE,   color_size,
                          EGL_RED_SIZE,        color_size,         EGL_ALPHA_SIZE,   color_size,
                          EGL_DEPTH_SIZE,      depth_size,         EGL_NONE};
    if (attrs == nullptr)
        attrs = backup_attrs;
    if (eglChooseConfig(display, attrs, configs, count, &count) == EGL_FALSE)
        return eglGetError();
    return 0;
}

egl_surface_owner_t::egl_surface_owner_t(EGLDisplay display, EGLConfig config, EGLSurface surface) noexcept
    : display{display}, config{config}, surface{surface} {
    spdlog::debug("{}: {}", "egl_surface_owner_t", surface);
}
egl_surface_owner_t::~egl_surface_owner_t() noexcept {
    if (eglDestroySurface(display, surface) == EGL_FALSE) {
        auto ec = eglGetError();
        spdlog::error("{}: {:#x}", "eglDestroySurface", ec);
        return;
    }
    spdlog::debug("{}: {}", "eglDestroySurface", surface);
}

EGLint egl_surface_owner_t::get_size(EGLint& width, EGLint& height) noexcept {
    eglQuerySurface(display, surface, EGL_WIDTH, &width);
    eglQuerySurface(display, surface, EGL_HEIGHT, &height);
    if (auto ec = eglGetError(); ec != EGL_SUCCESS) {
        spdlog::error("{}: {:#x}", "eglQuerySurface", ec);
        return ec;
    }
    return 0;
}

EGLSurface egl_surface_owner_t::handle() const noexcept {
    return surface;
}

egl_context_t::egl_context_t(EGLDisplay display, EGLContext share_context) noexcept : display{display} {
    if (eglInitialize(display, versions + 0, versions + 1) == false) {
        auto ec = eglGetError();
        spdlog::error("{}: {:#x}", "eglInitialize", ec);
        return;
    }
    spdlog::debug("EGLDisplay {} {}.{}", display, versions[0], versions[1]);

    // acquire EGLConfigs
    EGLint num_config = 1;
    if (auto ec = get_configs(display, configs, num_config, nullptr)) {
        spdlog::error("{}: {:#x}", "eglChooseConfig", ec);
        return;
    }

    // create context for OpenGL ES 3.0+
    EGLint attrs[]{EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};
    if (context = eglCreateContext(display, configs[0], share_context, attrs); context != EGL_NO_CONTEXT)
        spdlog::debug("EGL create: context {} {}", context, share_context);
}

egl_context_t::~egl_context_t() noexcept {
    spdlog::debug(__FUNCTION__);
    destroy();
}

EGLConfig egl_context_t::config() const noexcept {
    return configs[0];
}

EGLint egl_context_t::resume(EGLSurface es_surface, EGLConfig) noexcept {
    if (context == EGL_NO_CONTEXT)
        return EGL_NOT_INITIALIZED;
    if (es_surface == EGL_NO_SURFACE)
        return GL_INVALID_VALUE;
    surface = es_surface;
    spdlog::debug("EGL current: {}/{} {}", surface, surface, context);
    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        auto ec = eglGetError();
        spdlog::error("{}: {:#x}", "eglMakeCurrent", ec);
        return ec;
    }
    return 0;
}

EGLint egl_context_t::suspend() noexcept {
    if (context == EGL_NO_CONTEXT)
        return EGL_NOT_INITIALIZED;
    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
        auto ec = eglGetError();
        spdlog::error("{}: {:#x}", "eglMakeCurrent", ec);
    }
    surface = EGL_NO_SURFACE;
    return 0;
}

void egl_context_t::destroy() noexcept {
    if (display == EGL_NO_DISPLAY) // already terminated
        return;

    // unbind surface and context
    spdlog::debug("EGL current: EGL_NO_SURFACE/EGL_NO_SURFACE EGL_NO_CONTEXT");
    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
        auto ec = eglGetError();
        spdlog::error("{}: {:#x}", "eglMakeCurrent", ec);
        return;
    }
    // destroy known context
    if (context != EGL_NO_CONTEXT) {
        spdlog::warn("EGL destroy: context {}", context);
        if (eglDestroyContext(display, context) == EGL_FALSE) {
            auto ec = eglGetError();
            spdlog::error("{}: {:#x}", "eglDestroyContext", ec);
            return;
        }
        context = EGL_NO_CONTEXT;
    }
    // @todo EGLDisplay's lifecycle can be managed outside of this class.
    //       it had better forget about it rather than `eglTerminate`
    //// terminate EGL
    //spdlog::warn("EGL terminate: {}", display);
    //if (eglTerminate(display) == EGL_FALSE)
    //    report_egl_error("eglTerminate", eglGetError());
    display = EGL_NO_DISPLAY;
}

EGLint egl_context_t::swap() noexcept {
    if (eglSwapBuffers(display, surface))
        return EGL_SUCCESS;
    switch (const auto ec = eglGetError()) {
    case EGL_BAD_CONTEXT:
    case EGL_CONTEXT_LOST:
        destroy();
        [[fallthrough]];
    default:
        return ec; // EGL_BAD_SURFACE and the others ...
    }
}

EGLContext egl_context_t::handle() const noexcept {
    return context;
}

bool check_extensions(std::vector<std::string>& output) noexcept {
    std::vector<std::string> extensions{};
    if (auto txt = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS))) {
        const auto txtlen = strlen(txt);
        auto offset = 0u;
        for (auto i = 0u; i < txtlen; ++i) {
            if (isspace(txt[i]) == false)
                continue;
            extensions.emplace_back(txt + offset, i - offset);
            offset = ++i;
        }
        std::swap(output, extensions);
        return true;
    } else {
        // for OpenGL 4.x+
        glGetError(); // discard error from glGetString(GL_EXTENSIONS)
        GLint count = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        for (auto i = 0; i < count; ++i) {
            auto name = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
            extensions.emplace_back(name);
        }
        std::swap(output, extensions);
        return count > 0;
    }
}

bool get_opengl_version(GLint& major, GLint& minor) noexcept {
    auto const txt = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (txt == nullptr)
        return false;
    [[maybe_unused]] const auto buflen = strlen(txt); // expect ~50
    char buf[50]{};
    auto const count = sscanf_s(txt, "%s %s %d.%d", buf, 49, buf + 7, 42, &major, &minor);
    return count == 4;
}

class opengl_error_category_t final : public std::error_category {
    const char* name() const noexcept override {
        return "OpenGL";
    }
    std::string message(int ec) const override {
        constexpr auto bufsz = 40;
        char buf[bufsz]{};
        const auto len = snprintf(buf, bufsz, "error %5d(%4x)", ec, ec);
        return {buf, static_cast<size_t>(len)};
    }
};

std::error_category& get_opengl_category() noexcept {
    static opengl_error_category_t instance{};
    return instance;
};

#if defined(__APPLE__)
constexpr auto order = GL_BGRA;
#else
constexpr auto order = GL_RGBA;
#endif

tex2d_owner_t::tex2d_owner_t(GLsizei w, GLsizei h) noexcept(false) {
    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
    if (w > max || h > max)
        throw std::invalid_argument{"requested texture size is too big"};
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    constexpr auto level = 0, border = 0;
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, border, order, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // eglBindTexImage( display, surface, EGL_BACK_BUFFER ) ...
    glBindTexture(GL_TEXTURE_2D, 0);
}
tex2d_owner_t::~tex2d_owner_t() noexcept {
    glDeleteTextures(1, &tex);
}

GLuint tex2d_owner_t::handle() const noexcept {
    return tex;
}

GLenum tex2d_owner_t::update(GLsizei w, GLsizei h, const void* ptr) noexcept(false) {
    constexpr auto level = 0, border = 0;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, border, order, GL_UNSIGNED_BYTE, ptr);
    return glGetError();
}

fbo_owner_t::fbo_owner_t(GLuint tex) noexcept {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
}
fbo_owner_t::~fbo_owner_t() noexcept {
    glDeleteFramebuffers(1, &fbo);
}

GLuint fbo_owner_t::handle() const noexcept {
    return fbo;
}

pbo_reader_t::pbo_reader_t(GLuint length) noexcept(false) : pbos{}, length{length}, offset{} {
    GLint minor = 0;
    if (get_opengl_version(version, minor) == false)
        throw std::runtime_error{"Failed to query OpenGL version"};
    glGenBuffers(capacity, pbos);
    if (GLint ec = glGetError(); ec != GL_NO_ERROR)
        throw std::system_error{ec, get_opengl_category(), "glGenBuffers"};
    for (auto i = 0u; i < capacity; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, length, nullptr, GL_STREAM_READ);
        // glBufferData(GL_PIXEL_PACK_BUFFER, length, nullptr, GL_DYNAMIC_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

pbo_reader_t::~pbo_reader_t() noexcept {
    // delete and report if error generated
    glDeleteBuffers(capacity, pbos);
    if (auto ec = glGetError())
        spdlog::error("{} {}", __FUNCTION__, get_opengl_category().message(ec));
}

uint32_t pbo_reader_t::get_length() const noexcept {
    return length;
}

GLenum pbo_reader_t::pack(uint16_t idx, GLuint, const GLint frame[4], GLenum format, GLenum type) noexcept {
    if (idx >= capacity)
        return GL_INVALID_VALUE;
    //if (length < (frame[2] - frame[0]) * (frame[3] - frame[1]) * 4)
    //    return GL_OUT_OF_MEMORY;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[idx]);
    glReadPixels(frame[0], frame[1], frame[2], frame[3], format, type, reinterpret_cast<void*>(offset));
    if (auto ec = glGetError())
        return ec; // probably GL_OUT_OF_MEMORY?
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return glGetError();
}

GLenum pbo_reader_t::map_and_invoke(uint16_t idx, reader_callback_t callback, void* user_data) noexcept {
    if (idx >= capacity)
        return GL_INVALID_VALUE;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[idx]);
    if (version > 2) {
        if (const void* ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, offset, length, GL_MAP_READ_BIT)) {
            callback(user_data, ptr, length);
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
    }
    // else {
    //     if (const void* ptr = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)) {
    //         callback(user_data, ptr, length);
    //         glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    //     }
    // }
    return glGetError();
}

pbo_writer_t::pbo_writer_t(GLuint length) noexcept(false) : pbos{}, length{length} {
    GLint minor = 0;
    if (get_opengl_version(version, minor) == false)
        throw std::runtime_error{"Failed to query OpenGL version"};
    glGenBuffers(capacity, pbos);
    if (GLint ec = glGetError(); ec != GL_NO_ERROR)
        throw std::system_error{ec, get_opengl_category(), "glGenBuffers"};
    for (auto i = 0u; i < capacity; ++i) {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, length, nullptr, GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

pbo_writer_t::~pbo_writer_t() noexcept {
    // delete and report if error generated
    glDeleteBuffers(capacity, pbos);
    if (auto ec = glGetError())
        spdlog::error("{} {}", __FUNCTION__, get_opengl_category().message(ec));
}

uint32_t pbo_writer_t::get_length() const noexcept {
    return length;
}

/// @todo GL_MAP_UNSYNCHRONIZED_BIT?
GLenum pbo_writer_t::map_and_invoke(uint16_t idx, writer_callback_t callback, void* user_data) noexcept {
    if (idx >= capacity)
        return GL_INVALID_VALUE;
    // 1 is for write (upload)
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[idx]);
    if (version > 2) {
        if (void* ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, length, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT)) {
            // ...
            callback(user_data, ptr, length);
        }
    }
    // else {
    //     if (void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY)) {
    //         // ...
    //         callback(user_data, ptr, length);
    //     }
    // }
    if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER) == false)
        spdlog::warn("unmap buffer failed: {}", pbos[idx]);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    return glGetError();
}

GLenum pbo_writer_t::unpack(uint16_t idx, GLuint tex2d, const GLint frame[4], //
                            GLenum format, GLenum type) noexcept {
    if (idx >= capacity)
        return GL_INVALID_VALUE;
    GLenum ec = GL_NO_ERROR;
    glBindTexture(GL_TEXTURE_2D, tex2d);
    if (ec = glGetError(); ec != GL_NO_ERROR)
        return ec;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[idx]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, frame[0], frame[1], frame[2], frame[3], format, type, 0);
    if (ec = glGetError(); ec != GL_NO_ERROR)
        spdlog::warn("tex sub image failed: {}", pbos[idx]);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    return ec ? ec : glGetError();
}
