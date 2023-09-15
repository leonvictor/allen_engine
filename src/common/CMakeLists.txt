include(FetchContent)
include(GenerateExportHeader)

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

target_compile_definitions(${LIB_NAME} PUBLIC 
    UUID_SYSTEM_GENERATOR
)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME} 
    PRIVATE
        include/${LIB_NAME}  
    INTERFACE
        include
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR} 
        ${STDUUID_INCLUDE_DIRS} # TODO: Make STUUID a target
)

target_link_libraries(${LIB_NAME} PUBLIC 
    glm utils enkiTS lz4_static xxHash::xxhash
)

generate_export_header(${LIB_NAME}
    BASE_NAME ALN_${LIB_NAME}
)

# ---- Dependencies

# eastl
# Note : We use kind of a hack where we do not use the targets populated by FetchContent. This is due to a circular dependency between
# eastl and the common lib. We use a custom eastl configuration in (external/EASTL/aln_eastl.cpp) which depends on memory.hpp
FetchContent_Declare(
    eastl
    GIT_REPOSITORY https://github.com/electronicarts/EASTL
    GIT_TAG 05f4b4aef33f2f3ded08f19fa97f5a27ff35ff9f
    GIT_SUBMODULES_RECURSE off
    GIT_SHALLOW on
    GIT_PROGRESS on
)
FetchContent_MakeAvailable(eastl)
get_target_property(eastl_SOURCE_DIR EASTL SOURCE_DIR)

# To fix that we just manually add eastl sources to common
target_sources(${LIB_NAME} PRIVATE
    external/EASTL/aln_eastl.cpp
    ${eastl_SOURCE_DIR}
)

target_include_directories(${LIB_NAME} PUBLIC
    external
    ${eastl_SOURCE_DIR}/include
    ${eastl_SOURCE_DIR}/test/packages/EABase/include/Common
    ${eastl_SOURCE_DIR}/test/packages/EAAssert/include # TODO: Configure so that eastl uses our own asserts
)

target_compile_definitions(${LIB_NAME} PUBLIC 
 #   EASTL_USER_CONFIG_HEADER="EASTL/aln_eastl.hpp"
    EASTL_USER_DEFINED_ALLOCATOR
    EASTL_ASSERT_ENABLED=0
)

# ---- Tests

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