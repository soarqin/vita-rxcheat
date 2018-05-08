cmake_minimum_required(VERSION 3.6)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(rcsvr)

# C11
set(CMAKE_C_STANDARD 11)

# vitasdk
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_subdirectory(libcheat)

add_executable(kernel
  kernel.c
)

set_target_properties(kernel PROPERTIES LINK_FLAGS "-nostdlib" COMPILE_FLAGS "-D__VITA_KERNEL__")

target_link_libraries(kernel
  taihenForKernel_stub
  SceSysrootForKernel_stub
  SceSysmemForDriver_stub
  SceModulemgrForDriver_stub
  SceThreadmgrForDriver_stub
  SceIofilemgrForDriver_stub
)

vita_create_self(rcsvr.skprx kernel UNSAFE CONFIG kernel.yml)

# debug option
option(RCSVR_DEBUG "Debug output through udp like libdebugnet" OFF)
if(RCSVR_DEBUG)
    set(DEBUG_DEFS "-DRCSVR_DEBUG")
    set(RCSVR_DEBUG_IP "" CACHE STRING "Debug machine ip")
    set(RCSVR_DEBUG_PORT 18194 CACHE STRING "Debug machine port")
    option(RCSVR_DEBUG_LINENO "Add file:lineno to debug output" OFF)
    if(RCSVR_DEBUG_LINENO)
        set(DEBUG_DEFS "${DEBUG_DEFS} -DRCSVR_DEBUG_LINENO")
    endif()
    if(RCSVR_DEBUG_IP)
        set(DEBUG_DEFS "${DEBUG_DEFS} -DRCSVR_DEBUG_IP=\\\"${RCSVR_DEBUG_IP}\\\"")
    endif()
    if(RCSVR_DEBUG_PORT)
        set(DEBUG_DEFS "${DEBUG_DEFS} -DRCSVR_DEBUG_PORT=${RCSVR_DEBUG_PORT}")
    endif()
endif()

add_executable(user
    main.c
    # network
    net.c net.h
    # memory search
    mem.c mem.h
    # trophy
    trophy.c trophy.h
    # cheat code
    cheat.c cheat.h
    # draw
    blit.c blit.h font_pgf.c
    # util
    util.c util.h
    liballoc.c liballoc.h
    # debug
    debug.c debug.h
    # kcp
    ../kcp/ikcp.c ../kcp/ikcp.h
)

target_include_directories(user PRIVATE ../kcp)

target_link_libraries(user
    cheat
    taihen_stub
    SceLibc_stub_weak
    SceLibKernel_stub_weak
    gcc
    SceSysmem_stub_weak
    SceSysmodule_stub_weak
    SceKernelThreadMgr_stub_weak
    SceKernelModulemgr_stub_weak
    SceAppMgr_stub_weak
    SceCtrl_stub_weak
    SceDisplay_stub_weak
    SceIofilemgr_stub_weak
    SceNet_stub_weak
    SceNetCtl_stub_weak
    ScePower_stub_weak
    SceRtc_stub_weak
    ScePgf_stub_weak
    SceNpTrophy_stub_weak
)

set_target_properties(user PROPERTIES LINK_FLAGS "-nostdlib" COMPILE_FLAGS "-DNDEBUG ${DEBUG_DEFS}")

vita_create_self(rcsvr.suprx user UNSAFE CONFIG user.yml)

add_custom_target(dist ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/rcsvr.skprx ${CMAKE_BINARY_DIR}/dist/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/rcsvr.suprx ${CMAKE_BINARY_DIR}/dist/
)
add_dependencies(dist rcsvr.suprx rcsvr.skprx)
