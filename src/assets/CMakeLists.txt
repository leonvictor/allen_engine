set(LIB_NAME assets)

add_library(${LIB_NAME}
    src/request.cpp
    src/asset_service.cpp
    
    src/module/module.cpp
    )

set(LIB_DEPENDENCIES
    ${STB_INCLUDE_DIR}
    common
    )

set(LIB_LINK_DEPENDENCIES reflection common lz4_static nlohmann_json::nlohmann_json)

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