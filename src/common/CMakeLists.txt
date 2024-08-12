include(FetchContent)
include(GenerateExportHeader)

set(LIB_NAME common)

add_library(${LIB_NAME}
    src/transform.cpp
    src/colors.cpp
    src/serialization/binary_archive.cpp
    src/uuid.cpp
    src/string_id.cpp
    src/maths/vec2.cpp
    src/maths/vec3.cpp 
    src/maths/vec4.cpp
    src/maths/quaternion.cpp
    src/maths/matrix4x4.cpp
    src/services/settings_registry_service.cpp
)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME} 
    PRIVATE
        include/${LIB_NAME}  
    INTERFACE
        include
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR} 
)

target_link_libraries(${LIB_NAME} 
    PUBLIC 
        glm
        TracyClient
        enkiTS 
        nlohmann_json::nlohmann_json
        stduuid
        lz4_static # TODO: move uses to .cpp and move to private
    PRIVATE 
        xxHash::xxhash
)

generate_export_header(${LIB_NAME}
    BASE_NAME ALN_${LIB_NAME}
)

# ---- Dependencies
set(BUILD_SHARED_LIBS_GLOBAL_VALUE ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

# eastl
FetchContent_Declare(
    eastl
    GIT_REPOSITORY https://github.com/electronicarts/EASTL
    GIT_SUBMODULES_RECURSE off
    GIT_SHALLOW on
    GIT_PROGRESS on
    SYSTEM
)

FetchContent_MakeAvailable(eastl)

target_compile_definitions(EASTL PUBLIC 
    EASTL_USER_DEFINED_ALLOCATOR
    EASTL_ASSERT_ENABLED=0
    EASTL_DEPRECATIONS_FOR_2024_APRIL=EA_DISABLED
)

target_sources(${LIB_NAME} PRIVATE external/EASTL/aln_eastl.cpp) # Custom allocator
target_link_libraries(${LIB_NAME} PUBLIC EASTL)

set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_GLOBAL_VALUE})

# ---- Tests

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        4e8d92bf02f7d1c8006a0e7a5ecabd8e62d98502 # V3.6.0 https://github.com/catchorg/Catch2/releases/tag/v3.6.0
    SYSTEM
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests test/transform.cpp)
target_link_libraries(tests PRIVATE ${LIB_NAME} Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include (CTest)
include(Catch)
catch_discover_tests(tests)