cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME_VAR imgui)
project(${PROJECT_NAME_VAR} VERSION 1.0.0 LANGUAGES C CXX)
# C++ Version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imgui
    # imgui
    src/imgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
    src/imgui_impl_opengl3.cpp
    src/imgui_impl_glfw.cpp
)

# ==========================================
# Link Libraries
# ==========================================

target_link_libraries(${PROJECT_NAME_VAR}
    glfw
    OpenGL::GL
)

target_include_directories(${PROJECT_NAME_VAR}
    PUBLIC include
)