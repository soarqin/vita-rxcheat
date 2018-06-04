project(kcp)

add_library(kcp STATIC ikcp.c ikcp.h)
set_target_properties(kcp PROPERTIES COMPILE_FLAGS "-DNDEBUG")
