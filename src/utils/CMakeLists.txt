set(LIB_NAME utils)

add_library(${LIB_NAME} SHARED
    src/files.cpp
    )

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME} 
    PRIVATE # Include directories inside this lib access the headers directly
        $<BUILD_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include/${LIB_NAME} ${LIB_INCLUDE_DIRS}>
    INTERFACE # Outside of this lib, refer to headers as libname/header.hpp
        $<BUILD_INTERFACE:include ${LIB_INCLUDE_DIRS}>
        $<INSTALL_INTERFACE:include ${LIB_INCLUDE_DIRS}>
)