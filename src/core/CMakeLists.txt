set(LIB_NAME core)

add_library(${LIB_NAME} SHARED
    src/world_systems/render_system.cpp

    src/services/time_service.cpp
    src/mesh.cpp
    
    src/components/mesh_component.cpp
    src/components/static_mesh_component.cpp
    src/components/skeletal_mesh_component.cpp
    src/components/camera.cpp
    src/components/light.cpp
    src/components/animation_player_component.cpp
    
    src/entity_systems/script.cpp
    src/entity_systems/camera_controller.cpp
    src/entity_systems/animation_system.cpp

    src/type_descriptors/assets.cpp
)

set(LIB_DEPENDENCIES 
    ${Vulkan_INCLUDE_DIRS}
    TracyClient
)

set(LIB_LINK_DEPENDENCIES 
    glm utils common entities graphics input anim
    assets reflection)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    INTERFACE
        $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})