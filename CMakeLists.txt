cmake_minimum_required(VERSION 2.8.12)

project(rxcheat)

set(CMAKE_CXX_STANDARD 14)

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
add_subdirectory(deps/gl3w)
add_subdirectory(deps/glfw)
add_subdirectory(deps/imgui)
add_subdirectory(deps/yaml-cpp)
add_subdirectory(client)

option(BUILD_CONVERTER "Build PSVITACHEAT to RxCheat code converter" OFF)
if (BUILD_CONVERTER)
    add_subdirectory(deps/libuv)
    add_subdirectory(converter)
endif ()
