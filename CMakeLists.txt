cmake_minimum_required(VERSION 3.17)

project(ALLEN VERSION 0.1)

set(SOURCE 
    src/main.cpp
    src/time_system.cpp
)

# Configure the config files

set(CONFIG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
set(DEFAULT_SHADERS_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(DEFAULT_ASSETS_DIR ${PROJECT_SOURCE_DIR}/assets)
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
file(GLOB IMGUI_SOURCE "external/imgui/*.cpp")
set(SOURCE ${SOURCE} ${IMGUI_SOURCE} "external/imgui/backends/imgui_impl_vulkan.cpp" "external/imgui/backends/imgui_impl_glfw.cpp")
set(SOURCE ${SOURCE} "external/imgui_plugins/ImGuizmo.cpp")

add_executable(main ${SOURCE})

# Include ImGui
target_include_directories(main PUBLIC "external/imgui/backends")
target_include_directories(main PUBLIC "external/imgui")

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
set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/glfw/include)
target_include_directories(main PUBLIC ${GLFW_INCLUDE_DIR})
add_subdirectory(external/glfw)

# GLM
set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/glm)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_compile_definitions(GLM_GTX_hash)
target_include_directories(main PUBLIC ${GLM_INCLUDE_DIR})

# stb
set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/stb)
target_include_directories(main PUBLIC ${STB_INCLUDE_DIR})

# tinyobjloader
set(TINYOBJLOADER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/tinyobjloader)

# stduuids
set(STDUUID_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/stduuids)
# add_compile_definitions(UUID_SYSTEM_GENERATOR)
# target_include_directories(main PUBLIC ${STDUUID_INCLUDE_DIR})

# json
set(JSON_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/json)

# Other external headers
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
target_include_directories(main PUBLIC ${EXTERNAL_DIR})

# Include submodules
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

target_link_libraries(main PUBLIC glfw ${GLFW_LIBRARIES} shaderc ${Vulkan_LIBRARIES} imm32 utils input graphics entities core)



