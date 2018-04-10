cmake_minimum_required(VERSION 2.8.12)

project(rcsvr_client)

if (MSVC)
    set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            )
    foreach(CompilerFlag ${CompilerFlags})
      string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()
add_subdirectory(gl3w)
add_subdirectory(glfw)
add_subdirectory(imgui)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(rcsvr_client WIN32
    main.cpp
    imgui_impl_glfw_gl3.cpp imgui_impl_glfw_gl3.h
    command.cpp command.h
    net.cpp net.h
    ../kcp/ikcp.c ../kcp/ikcp.h
    )

target_include_directories(rcsvr_client PRIVATE glfw/include PRIVATE imgui PRIVATE ../kcp)
target_link_libraries(rcsvr_client imgui glfw ${GLFW_PKG_DEPS} ${GLFW_PKG_LIBS} gl3w ws2_32)
