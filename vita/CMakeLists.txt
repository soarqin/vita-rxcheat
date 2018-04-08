cmake_minimum_required(VERSION 3.6)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(PROJECT_NAME rcsvr)

project(${PROJECT_NAME})

# C11
set(CMAKE_C_STANDARD 11)
# C++14
set(CMAKE_CXX_STANDARD 14)

# vitasdk
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")
# remove assert symbol from kcp
add_definitions(-DNDEBUG)

# debug option
option(RCSVR_DEBUG "Debug output through udp like libdebugnet" OFF)
if(RCSVR_DEBUG)
    add_definitions(-DRCSVR_DEBUG)
    set(RCSVR_DEBUG_IP "" CACHE STRING "Debug machine ip")
    set(RCSVR_DEBUG_PORT 18194 CACHE STRING "Debug machine port")
    option(RCSVR_DEBUG_LINENO "Add file:lineno to debug output" OFF)
    if(RCSVR_DEBUG_LINENO)
        add_definitions(-DRCSVR_DEBUG_LINENO)
    endif()
    if(RCSVR_DEBUG_IP)
        add_definitions(-DRCSVR_DEBUG_IP="${RCSVR_DEBUG_IP}")
    endif()
    if(RCSVR_DEBUG_PORT)
        add_definitions(-DRCSVR_DEBUG_PORT=${RCSVR_DEBUG_PORT})
    endif()
endif()

add_executable(${PROJECT_NAME}
    main.c
    net.c net.h
    mem.c mem.h
    util.c util.h
    # debug
    debug.c debug.h
    # kcp
    ikcp.c ikcp.h
)

target_link_libraries(${PROJECT_NAME}
  taihen_stub
  SceLibc_stub_weak
  SceLibKernel_stub_weak
  gcc
  SceSysmem_stub_weak
  SceSysmodule_stub_weak
  SceKernelThreadmgr_stub_weak
  SceKernelModulemgr_stub_weak
  SceAppMgr_stub_weak
  SceCtrl_stub_weak
  SceIofilemgr_stub_weak
  SceNet_stub_weak
  SceNetCtl_stub_weak
  ScePower_stub_weak
  SceRtc_stub_weak
  kuio_stub_weak
)

set_property(TARGET ${PROJECT_NAME} PROPERTY LINK_FLAGS -nostdlib)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME} CONFIG exports.yml)
