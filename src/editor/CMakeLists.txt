set(LIB_NAME editor)

add_library(${LIB_NAME}
    src/reflected_types/reflected_type_editor.cpp
    src/reflected_types/primitive_type_editors.cpp

    src/editor.cpp
    src/assets_browser.cpp
    src/entity_inspector.cpp
    src/editor_window.cpp
 
    src/assets/animation_clip_workspace.cpp

    src/assets/animation_graph/animation_graph_workspace.cpp
    src/assets/animation_graph/editor_animation_graph_node.cpp
    src/assets/animation_graph/nodes/control_parameter_editor_nodes.cpp
    src/assets/animation_graph/nodes/parameter_reference_editor_node.cpp
    src/assets/animation_graph/nodes/float_editor_nodes.cpp
    src/assets/animation_graph/nodes/bool_editor_nodes.cpp
    src/assets/animation_graph/nodes/pose_editor_node.cpp
    src/assets/animation_graph/nodes/animation_clip_editor_node.cpp
    src/assets/animation_graph/nodes/blend_editor_node.cpp
    src/assets/animation_graph/nodes/state_machine_editor_node.cpp
    src/assets/animation_graph/nodes/transition_editor_node.cpp
    src/assets/animation_graph/nodes/state_editor_node.cpp
    src/assets/animation_graph/nodes/event_condition_editor_node.cpp
    src/assets/animation_graph/editor_animation_graph.cpp   
    src/assets/animation_graph/editor_animation_state_machine.cpp

    src/graph/editor_graph.cpp 
    src/graph/editor_graph_node.cpp
    src/graph/conduit.cpp

    src/module/module.cpp
    
    ${IMGUI_SOURCE}
)

set(LIB_DEPENDENCIES ${IMGUI_INCLUDE_DIRS})

set(LIB_LINK_DEPENDENCIES reflection assets core entities anim fmt::fmt)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    INTERFACE
        $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})