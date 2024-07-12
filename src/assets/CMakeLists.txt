set(LIB_NAME assets)

add_library(${LIB_NAME}
    src/loader.cpp
    src/request.cpp
    src/asset_service.cpp
    
    src/module/module.cpp
)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME}
    SYSTEM PUBLIC
        ${STB_INCLUDE_DIR}
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >
)

target_link_libraries(${LIB_NAME} 
    PUBLIC
        reflection 
        Vulkan::Vulkan
        common
        stduuid
        graphics
    PRIVATE
        lz4_static
)