cmake_minimum_required(VERSION 3.24)

project(
    Apex3D
    VERSION 0.1.0
    DESCRIPTION "Interactive Formula 1 track and telemetry visualiser"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Keep GLFW focused on the library needed by Apex3D.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.5.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# GLAD is generated once and kept in the repository so contributors do not
# need Python packages or a second dependency download during configuration.
add_library(glad_gl_core_33 STATIC
    third_party/glad/src/gl.c
)
target_include_directories(glad_gl_core_33 PUBLIC third_party/glad/include)

add_executable(Apex3D
    src/main.cpp
)

target_link_libraries(Apex3D
    PRIVATE
        glfw
        glad_gl_core_33
)

target_compile_definitions(Apex3D PRIVATE GLFW_INCLUDE_NONE)

if(MSVC)
    target_compile_options(Apex3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(Apex3D PRIVATE -Wall -Wextra -Wpedantic)
endif()
