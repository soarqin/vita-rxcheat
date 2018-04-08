cmake_minimum_required(VERSION 2.8.12)

project(rcsvr_client)

add_subdirectory(gl3w)
add_subdirectory(glfw)
add_subdirectory(imgui)

add_executable(rcsvr_client WIN32 main.cpp imgui_impl_glfw_gl3.cpp)

target_include_directories(rcsvr_client PRIVATE glfw/include PRIVATE imgui)
target_link_libraries(rcsvr_client imgui glfw ${GLFW_PKG_DEPS} ${GLFW_PKG_LIBS} gl3w)
