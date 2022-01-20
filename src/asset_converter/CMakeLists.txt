option(ASSET_CONVERTER_LIBRARY "Build the asset converter as library to link to (editor mode)" OFF)

message(STATUS "Building Asset Converter as a library (to be used in editor mode): ${ASSET_CONVERTER_LIBRARY}")

if(ASSET_CONVERTER_LIBRARY)
    add_library(asset_converter 
        src/converter.cpp)
else()
    add_executable(asset_converter
        src/converter.cpp
        src/cli.cpp)
endif()

target_compile_features(asset_converter PUBLIC cxx_std_20)

target_include_directories(asset_converter
    SYSTEM 
        PUBLIC  
        PRIVATE
            $<BUILD_INTERFACE:include/asset_converter >
            $<INSTALL_INTERFACE:include/asset_converter >
    INTERFACE
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >

)

target_link_libraries(asset_converter 
    PUBLIC
        common
        assets
    PRIVATE 
        assimp::assimp)