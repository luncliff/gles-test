cmake_minimum_required(VERSION 3.24)
project(gles-tests VERSION 0.0.0 LANGUAGES CXX)

message(STATUS "Using system: ${CMAKE_SYSTEM}")
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER}")

include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_library(EGL_LIBPATH NAMES libEGL EGL REQUIRED)
find_library(GLES_LIBPATH NAMES libGLESv3 GLESv3 libGLESv2 GLESv2 REQUIRED)
find_path(EGL_INCLUDE_DIR NAMES "EGL/egl.h" REQUIRED)

add_executable(test_suite
    gles.hpp
    gles.cpp
    gles_test.cpp
)

set_target_properties(test_suite
PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED true # Disable GNU extensions
)

target_include_directories(test_suite
PRIVATE
    ${EGL_INCLUDE_DIR}
    # $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
    # $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/externals/include>
    externals/include
    src
)

target_link_libraries(test_suite
PRIVATE
    fmt::fmt spdlog::spdlog
    ${EGL_LIBPATH} ${GLES_LIBPATH}
)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(WIN32)
        # clang-cl is not considered for now ...
    else()
        target_compile_options(test_suite
        PRIVATE
            -Wall -Wextra -stdlib=libc++
        )
    endif()

elseif(MSVC)
    # https://docs.microsoft.com/en-us/cpp/build/reference/source-charset-set-source-character-set
    target_compile_options(test_suite
    PRIVATE
        /W4 /Zc:__cplusplus /utf-8
    )

endif()

install(TARGETS test_suite
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CMakePackageConfigHelpers)
set(VERSION_FILE_PATH   ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake)
write_basic_package_version_file(${VERSION_FILE_PATH}
    VERSION             ${PROJECT_VERSION}
    COMPATIBILITY       SameMajorVersion
)
install(FILES           ${VERSION_FILE_PATH} 
        DESTINATION     ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}
)

enable_testing()
add_test(NAME test_9 COMMAND test_suite)
