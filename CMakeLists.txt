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

# todo: output variable for IFC target
function(add_module_interface NAME FILE)
    set(IFC_PATH "${PROJECT_BINARY_DIR}/${NAME}.ifc")
    set(IFC_TARGET "${NAME}-ifc")
    add_library(${IFC_TARGET} STATIC ${FILE})
    set_target_properties(${IFC_TARGET}
    PROPERTIES
        CXX_STANDARD 20
    )
    if(MSVC)
        target_compile_options(${IFC_TARGET}
        PUBLIC
            /experimental:module
        PRIVATE
            /interface # /ifcOnly # STATIC targets must generate .lib
        )
        install(FILES ${IFC_PATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()
endfunction()

# todo: more compiler support
function(add_module NAME TYPE IFC_TARGET)
    add_library(${NAME} ${TYPE})
    add_dependencies(${NAME} ${IFC_TARGET})
    set_target_properties(${NAME}
    PROPERTIES
        CXX_STANDARD 20
        # CXX_STANDARD_REQUIRED true # Disable GNU extensions
        WINDOWS_EXPORT_ALL_SYMBOLS false
    )
    if(MSVC)
        target_compile_options(${NAME}
        PUBLIC
            /experimental:module
        PRIVATE
            /showResolvedHeader # /internalPartition
        )
        target_link_options(${NAME}
        PRIVATE
            /subsystem:windows
        )
    endif()
endfunction()

function(module_include_directories NAME)
    cmake_parse_arguments(PARSE_ARGV 1 "arg" "" "IFC_TARGET" "PRIVATE;PUBLIC")
    if(NOT DEFINED arg_IFC_TARGET)
        set(arg_IFC_TARGET ${NAME}-ifc)
    endif()
    target_include_directories(${arg_IFC_TARGET}
    PRIVATE
        ${arg_PRIVATE} ${arg_PUBLIC}
    )
    target_include_directories(${NAME}
    PRIVATE
        ${arg_PRIVATE}
    PUBLIC
        ${arg_PUBLIC}
    )
endfunction()

# Generate IFC from primary module interface file
add_module_interface(gles
    gles_main.cpp
)

# Build with the IFC
add_module(gles SHARED gles-ifc)

target_sources(gles
PRIVATE
    gles_part_0.cpp
    gles_part_1.cpp
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
