cmake_minimum_required(VERSION 3.6)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(TARGET_NAME rcsvr)

add_subdirectory(kernel)

project(${TARGET_NAME})

# C11
set(CMAKE_C_STANDARD 11)

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

add_executable(${TARGET_NAME}
    main.c
    # network
    net.c net.h
    # memory search
    mem.c mem.h
    # trophy
    trophy.c trophy.h
    # draw
    font.c blit.c blit.h
    # util
    util.c util.h
    # debug
    debug.c debug.h
    # kcp
    ../kcp/ikcp.c ../kcp/ikcp.h
)

target_include_directories(${TARGET_NAME} PRIVATE ../kcp PRIVATE kernel)

target_link_libraries(${TARGET_NAME}
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
    SceDisplay_stub_weak
    SceIofilemgr_stub_weak
    SceNet_stub_weak
    SceNetCtl_stub_weak
    ScePower_stub_weak
    SceRtc_stub_weak
    SceNpTrophy_stub_weak
    ${CMAKE_BINARY_DIR}/kernel/stubs/librcsvr_stub_weak.a
)

set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS -nostdlib)
add_dependencies(${TARGET_NAME} stubs)

vita_create_self(${TARGET_NAME}.suprx ${TARGET_NAME} CONFIG exports.yml)

add_custom_target(dist ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.suprx ${CMAKE_BINARY_DIR}/dist/
    )
add_dependencies(dist ${TARGET_NAME}.suprx dist_kernel)
