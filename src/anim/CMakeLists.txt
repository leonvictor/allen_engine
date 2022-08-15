set(LIB_NAME anim)

add_library(${LIB_NAME} SHARED
    src/pose.cpp
    src/track.cpp
)

set(LIB_DEPENDENCIES  )

set(LIB_LINK_DEPENDENCIES glm common assets)

target_compile_features(${LIB_NAME} PUBLIC cxx_std_20)

target_include_directories(${LIB_NAME}
    SYSTEM PUBLIC
        ${LIB_DEPENDENCIES}
    PRIVATE
        $<BUILD_INTERFACE:include/${LIB_NAME} >
        $<INSTALL_INTERFACE:include/${LIB_NAME} >
    INTERFACE
        $<BUILD_INTERFACE:include >
        $<INSTALL_INTERFACE:include >

)

target_link_libraries(${LIB_NAME} PUBLIC ${LIB_LINK_DEPENDENCIES})