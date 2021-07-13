set(LIB_NAME core)

add_library(${LIB_NAME} SHARED
    src/camera.cpp
    )

set(LIB_DEPENDENCIES 
    ${GLM_INCLUDE_DIR}
    ${Vulkan_INCLUDE_DIRS}
    )

set(LIB_LINK_DEPENDENCIES utils entities graphics)

target_include_directories(${LIB_NAME} PRIVATE
    $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
)

target_include_directories(${LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
    $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})