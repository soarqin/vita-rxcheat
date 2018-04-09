cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(TARGET_NAME rcsvr_kernel)
project(${TARGET_NAME})

# C99
set(CMAKE_C_STANDARD 99)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_definitions(-D__VITA_KERNEL__)

add_executable(${TARGET_NAME}
  kio.c kio.h
  main.c
)

target_link_libraries(${TARGET_NAME}
  taihenForKernel_stub
  SceSysrootForKernel_stub
  SceSysmemForDriver_stub
  SceModulemgrForDriver_stub
  SceThreadmgrForDriver_stub
  SceIofilemgrForDriver_stub
)

vita_create_self(rcsvr.skprx ${TARGET_NAME} CONFIG exports.yml UNSAFE)
vita_create_stubs(stubs ${TARGET_NAME} exports.yml KERNEL)
add_custom_target(dist_kernel ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/rcsvr.skprx ${CMAKE_BINARY_DIR}/dist/
    )
add_dependencies(dist_kernel rcsvr.skprx)
