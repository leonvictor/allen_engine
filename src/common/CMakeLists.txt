set(LIB_NAME common)

add_library(${LIB_NAME} SHARED
    src/transform.cpp
    src/colors.cpp
    src/serialization/binary_archive.cpp
)

include(GenerateExportHeader)
generate_export_header(${LIB_NAME}
    BASE_NAME ALN_${LIB_NAME}
)

set(LIB_LINK_DEPENDENCIES glm utils enkiTS lz4_static)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:
            include/${LIB_NAME}  
            ${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:
            include/${LIB_NAME} 
            ${CMAKE_CURRENT_BINARY_DIR}>
    INTERFACE
        $<BUILD_INTERFACE:
            include
            ${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:
            include     
            ${CMAKE_CURRENT_BINARY_DIR} >
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests test/transform.cpp)
target_link_libraries(tests PRIVATE ${LIB_NAME} Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include (CTest)
include(Catch)
catch_discover_tests(tests)