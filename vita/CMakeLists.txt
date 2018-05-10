cmake_minimum_required(VERSION 3.6)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(rcsvr)

# vitasdk
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

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

option(RCSVR_KERNEL_DEBUG "Debug output to file for kernel module" OFF)
if(RCSVR_KERNEL_DEBUG)
    set(KERNEL_DEBUG_DEFS "-DRCSVR_DEBUG")
    option(RCSVR_KERNEL_DEBUG_LINENO "Add file:lineno to debug output" OFF)
    if(RCSVR_KERNEL_DEBUG_LINENO)
        set(KERNEL_DEBUG_DEFS "${KERNEL_DEBUG_DEFS} -DRCSVR_DEBUG_LINENO")
    endif()
endif()

add_subdirectory(libcheat)

add_executable(kernel
    kernel.c
    kernel_api.c kernel_api.h
    kernel_debug.c kernel_debug.h
)

target_link_libraries(kernel
    taihenForKernel_stub
    taihenModuleUtils_stub
    SceSysrootForKernel_stub
    SceCpuForDriver_stub
    SceSysmemForDriver_stub
    SceModulemgrForDriver_stub
    SceThreadmgrForDriver_stub
    SceIofilemgrForDriver_stub
    SceCtrlForDriver_stub
)

if (RCSVR_KERNEL_DEBUG)
    target_link_libraries(kernel
        k
        gcc
    )
endif ()

set_target_properties(kernel PROPERTIES LINK_FLAGS "-nostdlib" COMPILE_FLAGS "-std=gnu99 -D__VITA_KERNEL__ ${KERNEL_DEBUG_DEFS}")

vita_create_self(rcsvr.skprx kernel UNSAFE CONFIG kernel.yml)
vita_create_stubs(kernel-stubs kernel kernel.yml KERNEL)

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
    # ui
    ui.c ui.h
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
    ${CMAKE_BINARY_DIR}/kernel-stubs/librcsvr_stub_weak.a
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
    SceAppUtil_stub_weak
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

set_target_properties(user PROPERTIES LINK_FLAGS "-nostdlib" COMPILE_FLAGS "-std=gnu11 -DNDEBUG ${DEBUG_DEFS}")
add_dependencies(user kernel-stubs)

vita_create_self(rcsvr.suprx user UNSAFE CONFIG user.yml)

add_custom_target(dist ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/rcsvr.skprx ${CMAKE_BINARY_DIR}/dist/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/rcsvr.suprx ${CMAKE_BINARY_DIR}/dist/
)
add_dependencies(dist rcsvr.suprx rcsvr.skprx)
