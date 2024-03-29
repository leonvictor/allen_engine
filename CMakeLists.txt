cmake_minimum_required(VERSION 3.17)

include(FetchContent)

project(ALLEN VERSION 0.1)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE 
    src/applications/glfw_application.cpp)

enable_testing()

# Create the tracing configuration
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} ReleaseWithTracing)
# Default compile flags to those of the Release config
set(CMAKE_CXX_FLAGS_RELEASEWITHTRACING ${CMAKE_CXX_FLAGS_RELEASE} CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASEWITHTRACING ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASEWITHTRACING ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASEWITHTRACING ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "")
mark_as_advanced(
    CMAKE_CXX_FLAGS_RELEASEWITHTRACING
    CMAKE_C_FLAGS_RELEASEWITHTRACING
    CMAKE_EXE_LINKER_FLAGS_RELEASEWITHTRACING
    CMAKE_SHARED_LINKER_FLAGS_RELEASEWITHTRACING
)

# set the MSVC runtime libary to dynamic
# should work but doesn't for some reason
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
# TODO: I commented this out because it added a semicolon to the option list that we do not want
# set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} " /MDd")
# set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} " /MD")
# set(CMAKE_CXX_FLAGS_RELEASEWITHTRACING ${CMAKE_CXX_FLAGS_RELEASEWITHTRACING} " /MD")

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(ALLEN_EDITOR "Build the editor" ON)

# Configure the config files
set(CONFIG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
set(DEFAULT_SHADERS_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(DEFAULT_ASSETS_DIR ${PROJECT_SOURCE_DIR}/assets)
set(FONTS_DIR ${PROJECT_SOURCE_DIR}/fonts)

configure_file(
    "${CONFIG_INCLUDE_DIRS}/config/path.h.in"
    "${CONFIG_INCLUDE_DIRS}/config/path.h"
)

# Include the config files from anywhere
include_directories(CONFIG_INCLUDE_DIRS)

# Put the .exe and .dll in the build directory's root so we can find them.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ALLEN_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ALLEN_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ALLEN_BINARY_DIR})

# TODO: ?
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Add Imgui to sources
file(GLOB IMGUI_SOURCE ${CMAKE_SOURCE_DIR}/external/imgui/*.cpp)
set(IMGUI_SOURCE ${IMGUI_SOURCE} ${CMAKE_SOURCE_DIR}/external/imgui/misc/cpp/imgui_stdlib.cpp)
set(SOURCE 
    ${SOURCE} 
    ${IMGUI_SOURCE}
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp 
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
)

# set(SOURCE ${SOURCE} "external/imgui_plugins/ImGuizmo.cpp")

set(IMGUI_INCLUDE_DIRS 
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/misc/cpp
    ${CMAKE_SOURCE_DIR}/external/imgui_plugins
)

set(IMGUI_COMPILE_DEFINITIONS
    IMNODES_USER_CONFIG
    IMGUI_DEFINE_MATH_OPERATORS
    IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
)

# TODO: Use target-based version
add_compile_definitions(${IMGUI_COMPILE_DEFINITIONS})

add_executable(main ${SOURCE})

add_compile_definitions(
        $<$<CONFIG:Debug>:ALN_DEBUG>
)

# fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG a33701196adfad74917046096bf5a2aa0ab0bb50 # https://github.com/fmtlib/fmt/releases/tag/9.1.0
)

FetchContent_MakeAvailable(fmt)

# enkiTS
set(ENKITS_BUILD_C_INTERFACE OFF CACHE INTERNAL "")
set(ENKITS_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(ENKITS_BUILD_SHARED ON CACHE INTERNAL "")
set(ENKITS_INSTALL OFF CACHE INTERNAL "")

# TODO: Use a fixed release when the CMakeFile patch makes it there
FetchContent_Declare(
  enkiTS
  GIT_REPOSITORY https://github.com/dougbinks/enkiTS
  GIT_TAG fd5a2603a0adf1884eee40dd081e30733a3b46dd # (https://github.com/dougbinks/enkiTS/pull/76)
)

FetchContent_MakeAvailable(enkiTS)

# Tracy
option(TRACY_ENABLE "" true)
option(TRACY_CALLSTACK "" true)
# option(TRACY_NO_EXIT "" true)
FetchContent_Declare(
    TracyClient
    GIT_REPOSITORY https://github.com/wolfpld/tracy
    GIT_TAG 897aec5b062664d2485f4f9a213715d2e527e0ca # v0.9.1
)

#add_compile_definitions(
#        $<$<CONFIG:Debug>:TRACY_NO_EXIT>
#)

FetchContent_MakeAvailable(TracyClient)

set_target_properties(TracyClient
    PROPERTIES
        EXCLUDE_FROM_DEFAULT_BUILD_RELEASE TRUE
        # EXCLUDE_FROM_DEFAULT_BUILD_DEBUG TRUE
        # EXCLUDE_FROM_DEFAULT_BUILD_MIN_SIZE_REL TRUE
        # EXCLUDE_FROM_DEFAULT_BUILD_REL_WITH_DEB_INFO TRUE
    )

# Include ImGui
target_include_directories(main SYSTEM PUBLIC ${IMGUI_INCLUDE_DIRS})

# Vulkan
# TODO: Make a vulkan target
add_compile_definitions(
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_NO_EXCEPTIONS
    VULKAN_HPP_NO_NODISCARD_WARNINGS
    VULKAN_HPP_NO_SETTERS
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL
)

find_package(Vulkan REQUIRED)
target_include_directories(main PUBLIC ${Vulkan_INCLUDE_DIRS})

# shaderc
add_library(Vulkan::shaderc UNKNOWN IMPORTED)
file(TO_CMAKE_PATH $ENV{VULKAN_SDK} VULKAN_SDK) # Translate env path to CMake style 
set_target_properties(Vulkan::shaderc PROPERTIES 
    IMPORTED_LOCATION ${VULKAN_SDK}/Lib/shaderc_shared.lib
    INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_SDK}/Include)

# GSL
set(GSL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/gsl)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_BUILD_INSTALL OFF CACHE INTERNAL "")

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e # https://github.com/glfw/glfw/releases/tag/3.3.8
)

FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # https://github.com/g-truc/glm/releases/tag/0.9.9.8
)

FetchContent_MakeAvailable(glm)

target_compile_definitions(glm INTERFACE
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_GTX_hash
)

target_compile_options(glm INTERFACE /wd26495)

# stb
set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/stb)
target_include_directories(main PUBLIC ${STB_INCLUDE_DIR})

# stduuids
set(STDUUID_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/stduuids)
# add_compile_definitions(UUID_SYSTEM_GENERATOR)
# target_include_directories(main PUBLIC ${STDUUID_INCLUDE_DIR})

# json
SET(JSON_BuildTests OFF)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)

FetchContent_MakeAvailable(json)

# assimp
MESSAGE(STATUS "Configuring assimp...")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "")
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE INTERNAL "")
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE INTERNAL "")

set(ASSIMP_NO_EXPORT ON CACHE INTERNAL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE INTERNAL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE INTERNAL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE INTERNAL "")

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG 9519a62dd20799c5493c638d1ef5a6f484e5faf1 # v5.2.5
)

FetchContent_MakeAvailable(assimp)

# lz4
set(LZ4_BUNDLED_MODE ON CACHE INTERNAL "")
set(LZ4_BUILD_CLI OFF CACHE INTERNAL "")
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE INTERNAL "")

FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4
  GIT_TAG 5ff839680134437dbf4678f3d0c7b371d84f4964 #  v1.9.4 
  SOURCE_SUBDIR build/cmake
)

FetchContent_MakeAvailable(lz4)

target_compile_options(lz4_static PRIVATE 
    /wd5045
    /wd4820
)

# xxhash
FetchContent_Declare(
    xxhash
    URL https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.1.tar.gz
    SOURCE_SUBDIR cmake_unofficial
)

FetchContent_MakeAvailable(xxhash)

target_compile_options(xxhash PRIVATE 
    /wd4820
)
# Other external headers
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
target_include_directories(main SYSTEM INTERFACE ${EXTERNAL_DIR})

# Include submodules
add_subdirectory(src/common)
add_subdirectory(src/reflection)
add_subdirectory(src/core)
add_subdirectory(src/entities)
add_subdirectory(src/graphics)
add_subdirectory(src/input)
add_subdirectory(src/assets)
add_subdirectory(src/asset_converter)
add_subdirectory(src/editor)
add_subdirectory(src/anim)

# target_include_directories(main PUBLIC TracyClient)
target_link_libraries(main PUBLIC 
    # $<$<CONFIG:ReleaseWithTracing>:TracyClient>
    fmt::fmt
    enkiTS
    common 
    reflection 
    glfw 
    input 
    graphics 
    entities 
    core
    editor
    anim
    assimp::assimp
    Vulkan::Vulkan
    Vulkan::shaderc
)

# note: Those are MSVC-only flags
target_compile_options(main PRIVATE 
    /wd4244 # 'argument': narrowing conversion, possible loss of data
    /wd4505 # unreferenced function with internal linkage has been removed
    /wd4267 # 'return': narrowing conversion, possible loss of data
    /wd4100 # unreferenced formal parameter
)