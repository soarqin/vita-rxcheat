cmake_minimum_required(VERSION 3.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 14)

project(rxcheat)

add_subdirectory(deps/libuv)
set_target_properties(uv-static PROPERTIES FOLDER deps)
add_subdirectory(deps/yaml-cpp)
set_target_properties(yaml-cpp PROPERTIES FOLDER deps)
add_subdirectory(deps/kcp)
set_target_properties(kcp PROPERTIES FOLDER deps)
add_subdirectory(deps/kcpnet)
set_target_properties(kcpnet PROPERTIES FOLDER deps)
add_subdirectory(rxclient)
set_target_properties(rxclient PROPERTIES FOLDER apps)

option(BUILD_CONVERTER "Build PSVITACHEAT to RxCheat code converter" OFF)
if (BUILD_CONVERTER)
    add_subdirectory(converter)
    set_target_properties(rxconv PROPERTIES FOLDER apps)
endif ()
