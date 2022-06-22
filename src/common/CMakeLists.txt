set(LIB_NAME common)

add_library(${LIB_NAME} SHARED
    src/transform.cpp
)

set(LIB_LINK_DEPENDENCIES glm utils)

target_include_directories(${LIB_NAME} 
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >
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