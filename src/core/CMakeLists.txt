set(LIB_NAME core)

add_library(${LIB_NAME}

    src/services/time_service.cpp
    src/mesh.cpp
    
    src/components/mesh_component.cpp
    src/components/static_mesh_component.cpp
    src/components/skeletal_mesh_component.cpp
    src/components/camera.cpp
    src/components/light.cpp
    src/components/animation_player_component.cpp
    src/components/animation_graph.cpp
    
    src/entity_systems/script.cpp
    src/entity_systems/camera_controller.cpp
    src/entity_systems/animation_system.cpp
    src/entity_systems/player_controller.cpp

    src/world_systems/world_rendering_system.cpp
    
    src/module/module.cpp
)

set(LIB_LINK_DEPENDENCIES 
    common 
    entities 
    graphics 
    input 
    anim
    assets 
    reflection 
    TracyClient)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})