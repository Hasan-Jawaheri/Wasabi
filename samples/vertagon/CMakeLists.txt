cmake_minimum_required(VERSION 3.15)

project(FPSGame)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/CMake")
include(LinkToWasabi)
include(GLSL)

set(WASABI_PATH "/home/hasan/Desktop/wasabi/build/dist")

file(GLOB_RECURSE SOURCES "src/FPS/*.cpp")
file(GLOB_RECURSE HEADERS "include/FPS/*.hpp")

if (WIN32)
    add_executable(fps_game WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(fps_game ${SOURCES} ${HEADERS})
endif()

target_include_directories(fps_game PRIVATE SYSTEM "include/")
link_target_to_wasabi(fps_game ${WASABI_PATH})

build_glsl(glsl-code ${VULKAN_SDK_PATH})
add_dependencies(fps_game glsl-code)
