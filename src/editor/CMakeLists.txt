set(LIB_NAME editor)

add_library(${LIB_NAME}
    src/reflected_types/reflected_type_editor.cpp
    src/reflected_types/primitive_type_editors.cpp

    src/editor.cpp
    src/assets_browser.cpp
    src/entity_inspector.cpp
    src/editor_window.cpp
    
    src/animation_graph/animation_graph_editor.cpp
    src/animation_graph/editor_graph_node.cpp

    src/animation_graph/nodes/control_parameter_editor_nodes.cpp
    src/animation_graph/nodes/pose_editor_node.cpp
    src/animation_graph/nodes/animation_clip_editor_node.cpp
    src/animation_graph/nodes/blend_editor_node.cpp

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