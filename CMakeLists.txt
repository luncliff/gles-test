cmake_minimum_required(VERSION 3.21)
project(gles-test VERSION 0.0 LANGUAGES CXX)

message(STATUS "Using system: ${CMAKE_SYSTEM}")
message(STATUS "Using compiler: ${CMAKE_CXX_COMPILER}")

include(GNUInstallDirs)
set(CMAKE_INSTALL_UCRT_LIBRARIES true)
string(COMPARE EQUAL ${CMAKE_BUILD_TYPE} Debug CMAKE_INSTALL_DEBUG_LIBRARIES)
include(InstallRequiredSystemLibraries)

find_package(Vulkan REQUIRED) # Vulkan::Vulkan
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

find_path(EGL_INCLUDE_DIR NAMES "EGL/egl.h" REQUIRED)
find_library(EGL_LIBPATH NAMES libEGL EGL REQUIRED)
find_library(GLES_LIBPATH NAMES libGLESv3 GLESv3 libGLESv2 GLESv2 REQUIRED)

find_package(ut CONFIG REQUIRED) # Boost::ut
# find_path(ut_INCLUDE_DIR NAMES "boost/ut.hpp" "ut-1.1.9/include/boost/ut.hpp" REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scripts)
include(msvc-cpp-modules)

# Generate IFC from primary module interface file
add_module_interface(gles
    src/gles_main.cpp
)

# Build with the IFC
add_module(gles SHARED gles-ifc)

target_sources(gles
PRIVATE
    src/gles_part_0.cpp
    src/gles_part_1.cpp
)

# Handle some complicated includes...
module_include_directories(gles
PRIVATE
    ${Vulkan_INCLUDE_DIRS}
PUBLIC
    ${EGL_INCLUDE_DIR}
)

target_link_libraries(gles
PRIVATE
    fmt::fmt-header-only spdlog::spdlog_header_only
    ${EGL_LIBPATH} ${GLES_LIBPATH} # Vulkan::Vulkan
    WindowsApp d3d11 d3dcompiler dxgi dxguid
)

install(TARGETS gles
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if(WIN32)
    install(FILES $<TARGET_PDB_FILE:gles> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()

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

add_executable(test_suite
    gles_test.cpp
)

set_target_properties(test_suite
PROPERTIES
    CXX_STANDARD 20
)

target_link_libraries(test_suite
PRIVATE
    gles Boost::ut
    ${EGL_LIBPATH}
    WindowsApp
)

install(TARGETS test_suite
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
