#[===[.md:
# msvc-cpp-modules

### Requirements

Visual Studio 2022 (or Preview)

### Description

Support some CMake functions to declare CMake targets for C++ 20 Modules.
Especially for MSVC(cl.exe) command line arguments.

```cmake
# Generate IFC from primary module interface file
add_module_interface(my_module my_module_main.cpp)

# Build with the IFC. Use `target_sources` to add module partition sources.
add_module(my_module SHARED my_module-ifc)

# Handle some complicated includes...
module_include_directories(my_module
PRIVATE
    # ...
PUBLIC
    # ...
)
```

### See Also

- https://learn.microsoft.com/en-us/cpp/cpp/modules-cpp
- https://devblogs.microsoft.com/cppblog/standard-c20-modules-support-with-msvc-in-visual-studio-2019-version-16-8/
- https://developercommunity.visualstudio.com/t/When-importing-a-C20-module-or-header-/1550846

#]===]
if(X_MSVC_CPP_MODULES_GUARD)
    return()
endif()
set(X_MSVC_CPP_MODULES_GUARD ON) # Guard variable

# todo: output variable for IFC target
# todo: CMake (verbose) messages
function(add_module_interface NAME FILE)
    # set/export the .ifc path
    get_filename_component(IFC_PATH "${PROJECT_BINARY_DIR}/${NAME}.ifc" ABSOLUTE)
    set(${NAME}_IFC_PATH ${IFC_PATH} PARENT_SCOPE)
    # CMake target's name will have a suffix `-ifc`
    set(IFC_TARGET "${NAME}-ifc")
    message(STATUS "Target '${IFC_TARGET}' will generate ${IFC_PATH}")

    # If possible, Visual Studio Utility target is prefered.
    # But in CMake script, STATIC target is much simple to manage target properties
    add_library(${IFC_TARGET} STATIC ${FILE})

    set_target_properties(${IFC_TARGET}
    PROPERTIES
        CXX_STANDARD 20
    )
    # todo: support clang-cl
    if(MSVC)
        target_compile_options(${IFC_TARGET}
        PUBLIC
            /experimental:module
        PRIVATE
            /interface
            # /ifcOnly # STATIC target must generate .lib
        )
    endif()
    install(FILES ${IFC_PATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endfunction()

# todo: more compiler support
# todo: CMake (verbose) messages
function(add_module NAME TYPE IFC_TARGET)
    add_library(${NAME} ${TYPE})
    add_dependencies(${NAME} ${IFC_TARGET})
    set_target_properties(${NAME}
    PROPERTIES
        CXX_STANDARD 20
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
    # todo: support clang-cl
    if(MSVC)
        target_compile_options(${NAME}
        PUBLIC
            /experimental:module
            # /internalPartition # note: support `module_sources`?
        )
        target_link_options(${NAME}
        PRIVATE
            /subsystem:windows
        )
    endif()
endfunction()

# todo: more argument descriptions, samples
# todo: CMake (verbose) messages
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
    # todo: support clang-cl
    if(MSVC)
        target_compile_options(${NAME}
        PRIVATE
            /showResolvedHeader
        )
    endif()
endfunction()
