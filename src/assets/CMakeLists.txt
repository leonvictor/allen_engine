set(LIB_NAME assets)

add_library(${LIB_NAME} SHARED
    src/asset_system/asset_system.cpp
    src/asset_system/texture_asset.cpp
    src/asset_converter/converter.cpp
    )

set(LIB_DEPENDENCIES
    ${JSON_INCLUDE_DIRS}
    ${STB_INCLUDE_DIR}
    )

set(LIB_LINK_DEPENDENCIES )

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_DEPENDENCIES}>
    INTERFACE
        $<BUILD_INTERFACE:include ${LIB_DEPENDENCIES}>
        $<INSTALL_INTERFACE:include ${LIB_DEPENDENCIES}>

)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})