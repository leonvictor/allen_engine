set(LIB_NAME assets)

add_library(${LIB_NAME}
    src/asset_system/asset_system.cpp
    src/asset_system/texture_asset.cpp
    src/asset_system/mesh_asset.cpp
    src/asset_system/material_asset.cpp
    src/asset_system/animation_clip_asset.cpp
    src/asset_system/skeletal_mesh_asset.cpp
    src/asset_system/skeleton_asset.cpp

    src/request.cpp
    src/manager.cpp
    
    src/type_descriptors/handle.cpp
    )

set(LIB_DEPENDENCIES
    ${JSON_INCLUDE_DIRS}
    ${STB_INCLUDE_DIR}
    common
    )

set(LIB_LINK_DEPENDENCIES reflection common lz4_static)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME}
    SYSTEM PUBLIC
        ${LIB_DEPENDENCIES}
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >

)

target_link_libraries(
    ${LIB_NAME} 
    PUBLIC ${LIB_LINK_DEPENDENCIES}
    )