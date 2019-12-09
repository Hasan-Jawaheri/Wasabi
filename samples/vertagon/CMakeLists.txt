cmake_minimum_required(VERSION 3.15)

project(Vertagon)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "CMakePredefinedTargets")

include(LinkToWasabi)
include(GLSL)

if (NOT WASABI_PATH)
    set(WASABI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build/dist/")
endif()
message(STATUS "Wasabi path: ${WASABI_PATH}")

file(GLOB_RECURSE SOURCES "src/vertagon/*.cpp")
file(GLOB_RECURSE GLSL "src/vertagon/*.glsl")
file(GLOB_RECURSE HEADERS "include/vertagon/*.hpp")

if (WIN32)
    add_executable(vertagon WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(vertagon ${SOURCES} ${HEADERS})
endif()

target_include_directories(vertagon PRIVATE "include/")
link_target_to_wasabi(vertagon ${WASABI_PATH})
target_precompile_headers(vertagon PRIVATE
    "${WASABI_PATH}/include/Wasabi/Wasabi.hpp"
    "include/vertagon/common.hpp")
set_property(TARGET vertagon PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

build_glsl(TARGET glsl-code VULKAN ${VULKAN_SDK_PATH} CONVERTER ${CMAKE_SOURCE_DIR}/src/spv-converter.py SOURCES ${GLSL})
add_dependencies(vertagon glsl-code)
