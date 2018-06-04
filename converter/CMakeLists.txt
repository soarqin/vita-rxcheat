cmake_minimum_required(VERSION 2.8.12)

set(TARGET_NAME rxconv)

project(${TARGET_NAME})

add_definitions(-DUNICODE -D_UNICODE)

add_executable(${TARGET_NAME}
    main.cpp
    convert.cpp convert.h
    )

if(MSVC)
    set_target_properties(${TARGET_NAME} PROPERTIES
        DEBUG_POSTFIX "${LIB_TARGET_SUFFIX}d"
        RELEASE_POSTFIX "${LIB_TARGET_SUFFIX}"
        MINSIZEREL_POSTFIX "${LIB_TARGET_SUFFIX}"
        RELWITHDEBINFO_POSTFIX "${LIB_TARGET_SUFFIX}"
    )
endif()

target_include_directories(${TARGET_NAME} PRIVATE ../deps/libuv/include PRIVATE ../deps/kcpnet)
target_link_libraries(${TARGET_NAME} kcpnet)
if(WIN32)
    target_link_libraries(${TARGET_NAME} iphlpapi shlwapi ws2_32)
endif()
