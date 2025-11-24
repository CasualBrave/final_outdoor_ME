cmake_minimum_required(VERSION 3.5)
# ==========================================
# Configurable Variables
set(PROJECT_NAME_VAR CG2025)

# ==========================================
# Declare Project
project(${PROJECT_NAME_VAR} VERSION 1.0.0 LANGUAGES C CXX)
# C++ Version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# Add Subprojects
# ==========================================
# GLFW
add_subdirectory(external/glfw-3.4)
# Assimp
add_subdirectory(external/assimp)
# OpenGL
find_package(OpenGL REQUIRED)
# Imgui
add_subdirectory(external/imgui)
# Glad
add_subdirectory(external/glad)

# ==========================================
# Recursively collect all source and header files under ../src
file(GLOB_RECURSE PROJECT_SOURCES
    CONFIGURE_DEPENDS
    ../src/*.cpp
    ../src/*.c
    ../src/*.cc
    ../src/*.cxx
    ../src/*.h
    ../src/*.hpp
    ../src/*.hh
    ../src/*.hxx
)

# Add Executable
add_executable(${PROJECT_NAME_VAR} ${PROJECT_SOURCES})

# ==========================================
# Link Libraries
# ==========================================

target_link_libraries(${PROJECT_NAME_VAR}
    glfw
    glad
    assimp::assimp
    OpenGL::GL
    imgui
)

# ==========================================
# Header-only Libraries
# ==========================================

# For simplicity, we add header-only libraries in this way.
# For real-world project, it is still possible to generate library files for
# header-only libraries.
target_include_directories(${PROJECT_NAME_VAR}
    PRIVATE
        external/stb/include
        external/assimp/include
        external/glm
        external/tiny_obj_loader
)

# ==========================================
# Quality of life enhancement
# ==========================================

# VS startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME_VAR})

# Show source files in tree hierarchy in IDEs
source_group(TREE ${PROJECT_SOURCE_DIR}/.. FILES ${PROJECT_SOURCES})

# Set correct working directory when debugging
set_target_properties(${PROJECT_NAME_VAR} PROPERTIES
    XCODE_GENERATE_SCHEME TRUE
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
)