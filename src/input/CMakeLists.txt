set(LIB_NAME input)

add_library(
    ${LIB_NAME} SHARED
    src/input_action.cpp
    src/input_device.cpp
    src/input_system.cpp
    src/input_context.cpp
    src/mouse.cpp
    src/keyboard.cpp
    src/controls/input_control.cpp
    )

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_compile_features(input PUBLIC cxx_std_20)

set(LIB_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
)

set(LIB_DEPENDENCIES
    imm32
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_DEPENDENCIES})

target_include_directories(${LIB_NAME}
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include ${LIB_INCLUDE_DIRS}>
    )
