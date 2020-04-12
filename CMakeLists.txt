cmake_minimum_required(VERSION 3.17)

project(custom-engine VERSION 0.1)

find_package(Vulkan REQUIRED)

include_directories(${Vulkan_INCLUDE_DIRS})
include_directories("external/glfw/include")
include_directories("external/glm")
include_directories("external/stb")
include_directories("external/tinyobjloader")

set(GLFW_BUILD_EXAMPLE OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory(external/glfw)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(main src/main.cpp)

target_compile_features(main PUBLIC cxx_std_17)

target_link_libraries(main glfw)
target_link_libraries(main ${Vulkan_LIBRARIES})

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED True)

