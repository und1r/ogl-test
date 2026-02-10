cmake_minimum_required(VERSION 3.10)
project(ogl-test)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)

# Include directories
include_directories(src)
include_directories(src/deps/glad/include)

# Define all source files
set(SOURCES
    src/main.cpp
    src/core/Engine.cpp
    src/core/Input.cpp
    src/core/Camera.cpp
    src/core/Callbacks.cpp
    src/render/Renderer.cpp
    src/render/ShaderProgram.cpp
    src/render/Model.cpp
    src/render/ShadowMap.cpp
    src/render/Animation.cpp
    src/scene/Scene.cpp
    src/math/GeometryUtils.cpp
    src/math/Octree.cpp
    src/utils/RenderUtils.cpp
    src/deps/glad/src/gl.c
)

# Create the executable
add_executable(ogl-test ${SOURCES})

# Link libraries
# Note: "dl" is often needed for GLAD/loading dynamic libs on Linux
target_link_libraries(ogl-test
    OpenGL::GL
    glfw
    assimp
    dl
    pthread
)
