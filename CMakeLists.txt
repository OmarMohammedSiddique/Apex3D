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

# Compile the full-field telemetry table once instead of parsing it in every UI/test unit.
add_library(apex3d_race_data STATIC src/generated/monza_telemetry_2025.cpp)

add_executable(Apex3D
    src/main.cpp
)

target_link_libraries(Apex3D
    PRIVATE
        glfw
        glad_gl_core_33
        apex3d_race_data
)

target_compile_definitions(Apex3D PRIVATE GLFW_INCLUDE_NONE)

if(APPLE)
    target_link_libraries(Apex3D PRIVATE "-framework AudioToolbox")
endif()

enable_testing()
add_executable(Apex3DSceneTests tests/cp3_scene_tests.cpp)
add_test(NAME cp3_scene COMMAND Apex3DSceneTests)
add_executable(Apex3DRaceTests tests/cp3_race_tests.cpp)
add_test(NAME cp3_race COMMAND Apex3DRaceTests)
add_executable(Apex3DRenderTests tests/scene3d_tests.cpp)
add_test(NAME scene3d COMMAND Apex3DRenderTests)
add_executable(Apex3DWheelTests tests/cp3_wheel_tests.cpp)
add_test(NAME cp3_wheels COMMAND Apex3DWheelTests)

add_executable(Apex3DFieldTests tests/full_field_tests.cpp)
add_test(NAME full_field COMMAND Apex3DFieldTests)

add_executable(Apex3DWorldTests tests/world_detail_tests.cpp)
add_test(NAME world_detail COMMAND Apex3DWorldTests)

foreach(race_test Apex3DSceneTests Apex3DRaceTests Apex3DRenderTests Apex3DFieldTests Apex3DWorldTests)
    target_link_libraries(${race_test} PRIVATE apex3d_race_data)
endforeach()

if(MSVC)
    target_compile_options(Apex3D PRIVATE /W4 /permissive-)
    target_compile_options(Apex3DSceneTests PRIVATE /W4 /permissive-)
    target_compile_options(Apex3DRaceTests PRIVATE /W4 /permissive-)
    target_compile_options(Apex3DRenderTests PRIVATE /W4 /permissive-)
    target_compile_options(Apex3DWheelTests PRIVATE /W4 /permissive-)
    target_compile_options(Apex3DFieldTests PRIVATE /W4 /permissive-)
    target_compile_options(Apex3DWorldTests PRIVATE /W4 /permissive-)
else()
    target_compile_options(Apex3D PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Apex3DSceneTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Apex3DRaceTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Apex3DRenderTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Apex3DWheelTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Apex3DFieldTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(Apex3DWorldTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
