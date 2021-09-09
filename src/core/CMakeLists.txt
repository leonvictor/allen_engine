set(LIB_NAME core)

add_library(${LIB_NAME} SHARED
    src/camera.cpp
    src/mesh_renderer.cpp
    src/light.cpp
    src/render_system.cpp
    src/camera_controller.cpp
)

set(LIB_DEPENDENCIES 
    ${Vulkan_INCLUDE_DIRS}
)

set(LIB_LINK_DEPENDENCIES glm utils common entities graphics input)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    INTERFACE
        $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})