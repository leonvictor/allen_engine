set(LIB_NAME reflection)

add_library(${LIB_NAME} SHARED
    src/type_descriptors/primitives.cpp
    src/type_descriptors/std.cpp
    src/type_descriptors/glm.cpp
    src/reflection.cpp
    ${IMGUI_SOURCE}
    )

set(LIB_DEPENDENCIES ${IMGUI_INCLUDE_DIRS})

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)
target_link_libraries(${LIB_NAME} PUBLIC glm utils)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    INTERFACE
        $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>
)