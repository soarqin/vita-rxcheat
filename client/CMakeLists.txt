cmake_minimum_required(VERSION 2.8.12)

set(TARGET_NAME rcclient)

project(${TARGET_NAME})

add_definitions(-DUNICODE -D_UNICODE)

if(WIN32)
    set(RESOURCE_FILES rcclient.rc)
endif()

add_executable(${TARGET_NAME} WIN32
    main.cpp
    imgui_impl_glfw_gl3.cpp imgui_impl_glfw_gl3.h
    gui.cpp gui.h
    handler.cpp handler.h
    command.cpp command.h
    lang.cpp lang.h
    net.cpp net.h
    ../deps/kcp/ikcp.c ../deps/kcp/ikcp.h
    ${RESOURCE_FILES}
    )

if(MSVC)
	set_target_properties(${TARGET_NAME} PROPERTIES
		DEBUG_POSTFIX "${LIB_TARGET_SUFFIX}d"
		RELEASE_POSTFIX "${LIB_TARGET_SUFFIX}"
		MINSIZEREL_POSTFIX "${LIB_TARGET_SUFFIX}"
		RELWITHDEBINFO_POSTFIX "${LIB_TARGET_SUFFIX}"
	)
endif()

target_include_directories(${TARGET_NAME} PRIVATE ../deps/glfw/include PRIVATE ../deps/imgui PRIVATE ../deps/yaml-cpp/include PRIVATE ../deps/kcp)
target_link_libraries(${TARGET_NAME} imgui glfw ${GLFW_PKG_DEPS} ${GLFW_PKG_LIBS} gl3w yaml-cpp)
if(WIN32)
    target_link_libraries(${TARGET_NAME} iphlpapi shlwapi ws2_32)
endif()
