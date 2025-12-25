cmake_minimum_required(VERSION 3.24)
project(hp3d)

set(CMAKE_CXX_STANDARD 20)

# --- Find OpenGL (Fixes the error) ---
# This defines the target OpenGL::GL and variables like OPENGL_INCLUDE_DIR
find_package(OpenGL REQUIRED)

# --- 1. Dependencies (Automatic Download) ---
include(FetchContent)

# GLFW (Windowing)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLM (Math)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# GLAD (OpenGL Loader)
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad
        GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# --- 2. Source Files ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# --- 3. Executable ---
add_executable(hp3d ${SOURCES}
        src/app.cpp
        src/app.hpp
        src/arena.cpp
        src/arena.hpp
        src/camera.cpp
        src/camera.hpp)

# --- 4. Linking ---
# Note: OpenGL::GL usually handles includes automatically, but keeping explicit includes is fine.
target_include_directories(hp3d PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(hp3d PRIVATE glfw glad glm OpenGL::GL)

# Copy shaders to build directory so the executable can find them
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
