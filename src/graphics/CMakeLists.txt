include(GenerateExportHeader)

set(LIB_NAME graphics)

add_library(
    ${LIB_NAME}
    src/commandpool.cpp
    src/render_engine.cpp
    src/instance.cpp
    src/pipeline.cpp
    src/render_pass.cpp
    src/shaders.cpp
    src/subpass.cpp
    src/swapchain.cpp
    src/command_buffer.cpp
    src/resources/buffer.cpp
    src/resources/image.cpp
    src/resources/staging_buffer.cpp
    src/resources/allocation.cpp
    src/descriptor_allocator.cpp

     src/external/vma.cpp

    ${IMGUI_SOURCE}
)

set(LIB_INCLUDE_DIRS
    ${STB_INCLUDE_DIR}
    ${CONFIG_INCLUDE_DIRS}
    ${IMGUI_INCLUDE_DIRS}
)

set(LIB_DEPENDENCIES
    glfw
    common
    Vulkan::Vulkan
    Vulkan::shaderc 
    TracyClient
)

# Vulkan Memory Allocator
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/
    GIT_TAG a6bfc237255a6bac1513f7c1ebde6d8aed6b5191 # v3.0.1 https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/releases/tag/v3.0.1
)

FetchContent_MakeAvailable(vma)

target_compile_definitions(${LIB_NAME}
    PRIVATE
        VULKAN_HPP_STORAGE_SHARED_EXPORT
    PUBLIC
        VULKAN_HPP_STORAGE_SHARED
)

target_link_libraries(${LIB_NAME} 
    PUBLIC
        ${LIB_DEPENDENCIES}
        VulkanMemoryAllocator
)

target_include_directories(${LIB_NAME}
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include ${LIB_INCLUDE_DIRS}>
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR} 
)

generate_export_header(${LIB_NAME}
    BASE_NAME ALN_${LIB_NAME}
)