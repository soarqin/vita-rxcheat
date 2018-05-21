project(imgui)

add_definitions(-DUNICODE -D_UNICODE)
add_library(imgui STATIC imgui.cpp imgui_draw.cpp)
