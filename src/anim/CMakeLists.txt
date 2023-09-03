set(LIB_NAME anim)

add_library(${LIB_NAME} SHARED
    src/pose.cpp
    src/track.cpp
    src/sync_track.cpp
    
    src/graph/runtime_graph_node.cpp
    src/graph/passthrough_node.cpp
    src/graph/pose_node.cpp
    src/graph/nodes/control_parameter_nodes.cpp
    src/graph/nodes/float_nodes.cpp
    src/graph/nodes/bool_nodes.cpp
    src/graph/nodes/id_nodes.cpp
    src/graph/nodes/animation_clip_runtime_node.cpp
    src/graph/nodes/blend_runtime_node.cpp
    src/graph/nodes/state.cpp
    src/graph/nodes/transition.cpp
    src/graph/nodes/state_machine.cpp
    src/graph/nodes/event_condition_node.cpp

    src/graph/tasks/sample_task.cpp
    
    src/module/module.cpp
)

include(GenerateExportHeader)
generate_export_header(${LIB_NAME}
    BASE_NAME ALN_${LIB_NAME}
)

set(LIB_DEPENDENCIES  )

set(LIB_LINK_DEPENDENCIES glm common assets)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME}
    SYSTEM PUBLIC
        ${LIB_DEPENDENCIES}
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${CMAKE_CURRENT_BINARY_DIR}>
    INTERFACE
        $<BUILD_INTERFACE:include ${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include ${CMAKE_CURRENT_BINARY_DIR}>

)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})