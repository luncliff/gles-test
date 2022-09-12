#include <cstdio>
#include <filesystem>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "gles.hpp"

namespace fs = std::filesystem;

auto make_logger(const char* name, FILE* fout) noexcept(false) {
    using mutex_t = spdlog::details::console_nullmutex;
    using sink_t = spdlog::sinks::stdout_sink_base<mutex_t>;
    return std::make_shared<spdlog::logger>(name, std::make_shared<sink_t>(fout));
}

void test_egl_resume() {
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    egl_context_t context{display, EGL_NO_CONTEXT};

    EGLint attrs[]{EGL_WIDTH, 128, EGL_HEIGHT, 128, EGL_NONE};
    egl_surface_owner_t surface{display, context.config(), eglCreatePbufferSurface(display, context.config(), attrs)};

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

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    auto logger = make_logger("test", stdout);
    logger->set_pattern("%T.%e [%L] %8t %v");
    logger->set_level(spdlog::level::level_enum::debug);
    spdlog::set_default_logger(logger);
    auto cwd = fs::current_path();
    spdlog::debug("cwd: {}", cwd.generic_string());
    try {
        test_egl_resume();
    } catch (const std::system_error& ex) {
        auto&& ec = ex.code();
        spdlog::error("{}: {}", __func__, ec.message());
        return ec.value();
    } catch (const std::exception& ex) {
        spdlog::error("{}: {}", __func__, ex.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
