cmake_minimum_required(VERSION 3.17)

project(ALLEN VERSION 0.1)

set(SOURCE 
    src/main.cpp
)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Configure the config file
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
set(ARCHIVE_OUTPUT_DIRECTORY ${ALLEN_BINARY_DIR})

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

# Tracy
# TODO: Disable in prod.
add_compile_definitions(TRACY_ENABLE)
add_subdirectory(external/tracy)
target_link_libraries(main PUBLIC TracyClient)

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
add_subdirectory(external/glfw)

# GLM
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_compile_definitions(GLM_GTX_hash)
add_subdirectory(external/glm)

# Hack: set glm's include dir as system to ignore warnings (not our code)
get_target_property(_inc glm INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(glm SYSTEM INTERFACE ${_inc})

# stb
set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/stb)
target_include_directories(main PUBLIC ${STB_INCLUDE_DIR})

# tinyobjloader
add_subdirectory(external/tinyobjloader)

# stduuids
set(STDUUID_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/stduuids)
# add_compile_definitions(UUID_SYSTEM_GENERATOR)
# target_include_directories(main PUBLIC ${STDUUID_INCLUDE_DIR})

# json
set(JSON_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/json)

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

# Set C++ standard
set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_compile_features(main PUBLIC cxx_std_20)

target_link_libraries(main PUBLIC common reflection glfw shaderc ${Vulkan_LIBRARIES} imm32 utils input graphics entities core)

set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} " /EHsc /W4 /D_DEBUG /MDd")
message(CMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG})