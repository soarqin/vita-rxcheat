project(kcpnet)

add_library(kcpnet STATIC kcpclient.cpp kcpclient.h)
target_include_directories(kcpnet PRIVATE ../kcp ../libuv/include)
target_link_libraries(kcpnet PUBLIC kcp PUBLIC uv-static)
