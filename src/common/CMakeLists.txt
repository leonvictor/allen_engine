set(LIB_NAME common)

add_library(${LIB_NAME} SHARED
    src/transform.cpp
    src/colors.cpp
    src/serialization/binary_archive.cpp
    src/memory.cpp
    src/uuid.cpp
    src/string_id.cpp
    src/maths/vec2.cpp
    src/maths/vec3.cpp 
    src/maths/vec4.cpp
    src/maths/quaternion.cpp
    src/maths/matrix4x4.cpp
)

include(GenerateExportHeader)
generate_export_header(${LIB_NAME}
    BASE_NAME ALN_${LIB_NAME}
)

set(LIB_INCLUDE_DIRS     
    ${GSL_INCLUDE_DIRS}
    ${STDUUID_INCLUDE_DIRS}
)

set(LIB_LINK_DEPENDENCIES glm utils enkiTS lz4_static xxHash::xxhash)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

# Set target compile definitions
target_compile_definitions(${LIB_NAME} 
    PUBLIC UUID_SYSTEM_GENERATOR)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:
            include/${LIB_NAME}  
            ${CMAKE_CURRENT_BINARY_DIR} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:
            include/${LIB_NAME} 
            ${CMAKE_CURRENT_BINARY_DIR} ${LIB_INCLUDE_DIRS}>
    INTERFACE
        $<BUILD_INTERFACE:
            include
            ${CMAKE_CURRENT_BINARY_DIR} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:
            include     
            ${CMAKE_CURRENT_BINARY_DIR} ${LIB_INCLUDE_DIRS}>
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests test/transform.cpp "src/maths/vec3.cpp" "src/maths/quaternion.cpp" "src/maths/matrix4x4.cpp" "src/maths/vec4.cpp")
target_link_libraries(tests PRIVATE ${LIB_NAME} Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include (CTest)
include(Catch)
catch_discover_tests(tests)