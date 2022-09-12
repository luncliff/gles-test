// #include <boost/ut.hpp>
#include <cstdio>
#include <filesystem>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;
namespace testing = boost::ut;

auto make_logger(const char* name, FILE* fout) noexcept(false) {
    using mutex_t = spdlog::details::console_nullmutex;
    using sink_t = spdlog::sinks::stdout_sink_base<mutex_t>;
    return std::make_shared<spdlog::logger>(name, std::make_shared<sink_t>(fout));
}

int main(int argc, char* argv[]) {
    auto logger = make_logger("test", stdout);
    logger->set_pattern("%T.%e [%L] %8t %v");
    logger->set_level(spdlog::level::level_enum::debug);
    spdlog::set_default_logger(logger);
    auto cwd = fs::current_path();
    spdlog::debug("cwd: {}", cwd.generic_u8string());
    return 0;
}
