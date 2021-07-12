set(LIB_NAME graphics)

add_library(
    ${LIB_NAME} STATIC
    src/commandpool.cpp
    src/device.cpp
    src/instance.cpp
    src/pipeline.cpp
    src/render_pass.cpp
    src/shaders.cpp
    src/subpass.cpp
    src/swapchain.cpp
    src/window.cpp
    src/resources/buffer.cpp
    src/resources/image.cpp
    src/resources/texture.cpp
    src/resources/texture_cubemap.cpp
    )

set(LIB_INCLUDE_DIRS
    ${Vulkan_INCLUDE_DIRS} 
    ${GLFW_INCLUDE_DIR} 
    ${GLM_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
    ${CONFIG_INCLUDE_DIRS}
)

set(LIB_DEPENDENCIES
    ${Vulkan_LIBRARIES} utils input shaderc glfw ${GLFW_LIBRARIES}
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_DEPENDENCIES})

target_include_directories(${LIB_NAME}
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include ${LIB_INCLUDE_DIRS}>
    )
