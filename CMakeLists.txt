cmake_minimum_required(VERSION 3.17)

include(FetchContent)

project(ALLEN VERSION 0.1)

set(SOURCE 
    src/main.cpp)

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
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} " /MDd")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} " /MD")
set(CMAKE_CXX_FLAGS_RELEASEWITHTRACING ${CMAKE_CXX_FLAGS_RELEASEWITHTRACING} " /MD")

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

set(SOURCE ${SOURCE} "external/imgui_plugins/ImGuizmo.cpp")

set(IMGUI_INCLUDE_DIRS 
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/misc/cpp
    ${CMAKE_SOURCE_DIR}/external/imgui_plugins
)

add_executable(main ${SOURCE})

# fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG a33701196adfad74917046096bf5a2aa0ab0bb50 # https://github.com/fmtlib/fmt/releases/tag/9.1.0
)

FetchContent_MakeAvailable(fmt)

# enkiTS
set(ENKITS_BUILD_C_INTERFACE OFF)
set(ENKITS_BUILD_EXAMPLES OFF)
set(ENKITS_BUILD_SHARED ON)
set(ENKITS_INSTALL OFF)

# TODO: Use a fixed release when the CMakeFile patch makes it there
FetchContent_Declare(
  enkiTS
  GIT_REPOSITORY https://github.com/dougbinks/enkiTS
  GIT_TAG fd5a2603a0adf1884eee40dd081e30733a3b46dd # (https://github.com/dougbinks/enkiTS/pull/76)
)

FetchContent_MakeAvailable(enkiTS)

# Tracy
target_compile_definitions(main PRIVATE
    $<$<CONFIG:ReleaseWithTracing>:
        TRACY_ENABLE
        TRACY_ON_DEMAND
        TRACY_CALLSTACK
        ALN_ENABLE_TRACING>
)

FetchContent_Declare(
    TracyClient
    GIT_REPOSITORY https://github.com/wolfpld/tracy
    GIT_TAG a8511d357650282c6915f7aa9775e9c4945e42b5 # v0.8.2
)

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
add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
find_package(Vulkan REQUIRED)
target_include_directories(main PUBLIC ${Vulkan_INCLUDE_DIRS})
file(TO_CMAKE_PATH $ENV{VULKAN_SDK} VULKAN_SDK) # Translate env path to CMake style 

# shaderc
add_library(shaderc UNKNOWN IMPORTED)
set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/Lib/shaderc_shared.lib)
set_property(TARGET shaderc PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_SDK}/Include)

# GSL
set(GSL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/gsl)

# GLFW
set(GLFW_BUILD_EXAMPLE OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw
    GIT_TAG 7482de6071d21db77a7236155da44c172a7f6c9e # https://github.com/glfw/glfw/releases/tag/3.3.8
)

FetchContent_MakeAvailable(glfw)

# GLM
target_compile_definitions(main PUBLIC
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_GTX_hash
    )

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e # https://github.com/g-truc/glm/releases/tag/0.9.9.8
)

FetchContent_MakeAvailable(glm)

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
    GIT_URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)

FetchContent_MakeAvailable(json)

# assimp
MESSAGE(STATUS "Configuring assimp...")
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_NO_EXPORT ON)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG c8dafe0d2887242285c0080c6cbbea8c1f1c8094 # v5.2.4
)

FetchContent_MakeAvailable(assimp)

# lz4
set(LZ4_BUNDLED_MODE ON)
set(LZ4_BUILD_CLI OFF)
set(LZ4_BUILD_LEGACY_LZ4C OFF)

FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4
  GIT_TAG 5ff839680134437dbf4678f3d0c7b371d84f4964 #  v1.9.4 
  SOURCE_SUBDIR build/cmake
)

FetchContent_MakeAvailable(lz4)

# Other external headers
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
target_include_directories(main SYSTEM INTERFACE ${EXTERNAL_DIR})

# Include submodules
add_subdirectory(src/common)
add_subdirectory(src/reflection)
add_subdirectory(src/utils)
add_subdirectory(src/core)
add_subdirectory(src/entities)
add_subdirectory(src/graphics)
add_subdirectory(src/input)
add_subdirectory(src/assets)
add_subdirectory(src/asset_converter)
add_subdirectory(src/editor)
add_subdirectory(src/anim)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_compile_features(main PUBLIC cxx_std_20)

# target_include_directories(main PUBLIC TracyClient)
target_link_libraries(main PUBLIC 
    # $<$<CONFIG:ReleaseWithTracing>:TracyClient>
    fmt::fmt
    enkiTS
    common 
    reflection 
    glfw 
    shaderc 
    ${Vulkan_LIBRARIES} 
    imm32 
    utils 
    input 
    graphics 
    entities 
    core
    editor
    anim
    assimp::assimp)

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} " /Zi /EHsc /W4 /D_DEBUG /MDd")
message(CMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG})