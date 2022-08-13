set(LIB_NAME entities)
add_library(
    ${LIB_NAME} SHARED

    src/entity.cpp
    src/component.cpp
    src/entity_map.cpp
    src/spatial_component.cpp
    src/command.cpp
    src/world_entity.cpp
    src/world_system.cpp
    src/entity_system.cpp
    )

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME} 
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >
)

target_link_libraries(${LIB_NAME} 
    PRIVATE 
        glm
        utils
        fmt
    PUBLIC
        reflection
        common
        TracyClient
        assets
)