set(LIB_NAME entities)
add_library(
    ${LIB_NAME} SHARED

    src/entity.cpp
    src/component.cpp
    src/entity_collection.cpp
    src/entity_map.cpp
    src/spatial_component.cpp
    src/command.cpp
    src/transform.cpp
    )

set(LIB_INCLUDE_DIRS
    ${GLM_INCLUDE_DIR}
    )

set(LIB_LINK_DEPENDENCIES
    utils
)

target_include_directories(${LIB_NAME} 
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include ${LIB_INCLUDE_DIRS}>
)

target_link_libraries(${LIB_NAME} PRIVATE ${LIB_LINK_DEPENDENCIES})