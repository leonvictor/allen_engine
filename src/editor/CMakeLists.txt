set(LIB_NAME editor)

add_library(${LIB_NAME}
    src/editor.cpp

    # src/type_descriptors/asset_handle.cpp
    ${IMGUI_SOURCE}
)

set(LIB_DEPENDENCIES  ${IMGUI_INCLUDE_DIRS})

set(LIB_LINK_DEPENDENCIES reflection assets core entities)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    INTERFACE
        $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})