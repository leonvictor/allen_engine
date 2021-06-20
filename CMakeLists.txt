cmake_minimum_required(VERSION 3.17)

project(custom-engine VERSION 0.1)
# TODO: Find a better way to include all sources
set(SOURCE "src/main.cpp"
           "src/graphics/instance.cpp"
           "src/graphics/device.cpp" 
           "src/graphics/shaders.cpp"
           "src/graphics/commandpool.cpp"
           "src/graphics/resources/buffer.cpp"
           "src/graphics/resources/image.cpp"
           "src/graphics/resources/texture_cubemap.cpp"
           "src/graphics/resources/texture.cpp"
           "src/graphics/render_pass.cpp"
           "src/graphics/subpass.cpp"
           "src/graphics/swapchain.cpp"
           "src/graphics/pipeline.cpp"
           "src/graphics/window.cpp"
           "src/utils/uuid.cpp"
           "src/input/input_system.cpp"
           "src/input/input_device.cpp"
           "src/input/input_action.cpp"
           "src/input/controls/input_control.cpp")

add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
find_package(Vulkan REQUIRED)

# Translate env path to CMake style 
file(TO_CMAKE_PATH $ENV{VULKAN_SDK} VULKAN_SDK)

add_library(shaderc UNKNOWN IMPORTED)
set_target_properties(shaderc PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/Lib/shaderc_shared.lib)
set_property(TARGET shaderc PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${VULKAN_SDK}/Include)

include_directories(${Vulkan_INCLUDE_DIRS})
include_directories("external/glfw/include")
include_directories("external/glm")
include_directories("external/stb")
include_directories("external/tinyobjloader")
include_directories("external/imgui")
include_directories("external/imgui/backends")
include_directories("external")

file(GLOB IMGUI_SOURCE "external/imgui/*.cpp")
set(SOURCE ${SOURCE} ${IMGUI_SOURCE} "external/imgui/backends/imgui_impl_vulkan.cpp" "external/imgui/backends/imgui_impl_glfw.cpp")

set(SOURCE ${SOURCE} "external/imgui_plugins/ImGuizmo.cpp")

set(GLFW_BUILD_EXAMPLE OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")
add_subdirectory(external/glfw)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(main ${SOURCE})

target_compile_features(main PUBLIC cxx_std_20)

target_link_libraries(main glfw shaderc ${Vulkan_LIBRARIES} imm32)

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)

