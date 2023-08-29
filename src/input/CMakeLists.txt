set(LIB_NAME input)

add_library(
    ${LIB_NAME} SHARED

    src/input_action.cpp
    src/input_service.cpp
    src/input_context.cpp
    
    src/devices/mouse.cpp
    src/devices/keyboard.cpp
    
    src/input_control.cpp
    )

set(CMAKE_CXX_STANDARD 20) 
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_compile_features(input PUBLIC cxx_std_20)

set(LIB_DEPENDENCIES
    glm
    glfw
    imm32
    common
)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_DEPENDENCIES})

target_include_directories(${LIB_NAME}
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >
    )
