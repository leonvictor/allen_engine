set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(asset_system STATIC
"asset_system.cpp"
"asset_system.hpp"
"texture_asset.cpp"
"texture_asset.hpp")

target_include_directories(asset_system PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")