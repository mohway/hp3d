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

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG release
)
FetchContent_MakeAvailable(tinyobjloader)

# ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG v1.90.4-docking
)
FetchContent_MakeAvailable(imgui)

# Define ImGui sources manually since it doesn't have a CMakeLists.txt by default usually, 
# or we want specific backends.
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- 2. Source Files ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# --- 3. Executable ---
add_executable(hp3d ${SOURCES} ${IMGUI_SOURCES}
        src/app.cpp
        src/app.hpp
        src/arena.cpp
        src/arena.hpp
        src/camera.cpp
        src/camera.hpp
        src/engine/resource_manager.cpp
        src/engine/resource_manager.hpp
        src/engine/window.cpp
        src/engine/window.hpp
        src/engine/renderer.cpp
        src/engine/renderer.hpp
        src/engine/game_object.cpp
        src/engine/game_object.hpp
        src/engine/scene.cpp
        src/engine/scene.hpp
        src/game/level1.cpp
        src/game/level1.hpp
        src/engine/player_controller.cpp
        src/engine/player_controller.hpp
        src/engine/physics.cpp
        src/engine/physics.hpp
        src/engine/imgui_layer.cpp
        src/engine/imgui_layer.hpp)

# --- 4. Linking ---
# Note: OpenGL::GL usually handles includes automatically, but keeping explicit includes is fine.
target_include_directories(hp3d PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(hp3d PUBLIC ${stb_SOURCE_DIR} ${tinyobjloader_SOURCE_DIR})
target_include_directories(hp3d PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

target_link_libraries(hp3d PRIVATE glfw glad glm OpenGL::GL)

# Copy shaders to build directory so the executable can find them
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
