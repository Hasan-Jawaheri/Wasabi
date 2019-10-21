cmake_minimum_required(VERSION 3.16)

project(Wasabi)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/CMake")
include(WasabiHelpers)

#
# Find the vulkan installation
#
find_package(Vulkan)
if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else()
	message(STATUS "Vulkan library: " ${Vulkan_LIBRARY})
	message(STATUS "Vulkan include: " ${Vulkan_INCLUDE_DIRS})
    set(VULKAN_SDK_PATH ${Vulkan_INCLUDE_DIRS}/..)
endif()

#
# Build dependencies
#
set(DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
initialize_submodules()
build_open_al("OPENAL_DIR" "${DEPENDENCIES_DIR}")
build_bullet("BULLET_DIR" "${DEPENDENCIES_DIR}")
build_glfw("GLFW_DIR" "${DEPENDENCIES_DIR}")
build_assimp("ASSIMP_DIR" "${DEPENDENCIES_DIR}")
build_stb("STB_DIR" "${DEPENDENCIES_DIR}")
build_tinyfiledialogs("TFD_DIR" "${DEPENDENCIES_DIR}")

#
# Build the Wasabi library
#

# Source files
file(GLOB_RECURSE LIB_SOURCES "src/Wasabi/*.cpp")
file(GLOB_RECURSE LIB_GLSL "src/Wasabi/*.glsl")
file(GLOB_RECURSE LIB_HEADERS "include/Wasabi/*")

# Wasabi library
assign_source_group(${LIB_SOURCES} ${LIB_GLSL} ${LIB_HEADERS})
build_glsl(TARGET lib-glsl-code VULKAN ${VULKAN_SDK_PATH} CONVERTER ${CMAKE_SOURCE_DIR}/src/spv-converter.py SOURCES ${LIB_GLSL})
add_library(standalone-wasabi STATIC ${LIB_SOURCES} ${LIB_GLSL} ${LIB_HEADERS})
add_dependencies(standalone-wasabi lib-glsl-code)
target_compile_definitions(standalone-wasabi PRIVATE COMPILING_WASABI)
target_compile_definitions(standalone-wasabi PRIVATE AL_LIBTYPE_STATIC) # <-- this makes the OpenAL headers not do dllimport (on Windows)
target_include_directories(standalone-wasabi PRIVATE "include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM ${Vulkan_INCLUDE_DIRS})
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${OPENAL_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${BULLET_DIR}/src/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${GLFW_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${ASSIMP_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${STB_DIR}/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${TFD_DIR}/")
target_precompile_headers(standalone-wasabi PRIVATE
    "include/Wasabi/Core/WCore.hpp"
    "include/Wasabi/Core/VkTools/vulkanswapchain.hpp"
    "include/Wasabi/Core/VkTools/vulkantools.hpp"
    "include/Wasabi/Core/WError.hpp"
    "include/Wasabi/Core/WManager.hpp"
    "include/Wasabi/Core/WMath.hpp"
    "include/Wasabi/Core/WOrientation.hpp"
    "include/Wasabi/Core/WTimer.hpp"
    "include/Wasabi/Core/WUtilities.hpp"
    "include/Wasabi/Core/WBase.hpp"
    "include/Wasabi/Core/WCommon.hpp"
    "include/Wasabi/Core/WCompatibility.hpp")
enable_all_warnings(standalone-wasabi)

# Bundle the libraries together
bundle_static_library(
    TARGET standalone-wasabi
    BUNDLED_TARGET wasabi
    DEPENDENCIES tinyfiledialogs ex-common OpenAL glfw Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry BulletCollision BulletDynamics LinearMath assimp)
# Build the dist folder
build_dist(build-dist wasabi)

#
# Build the tests
#

# Source files
file(GLOB_RECURSE TEST_SOURCES "src/WasabiTest/*.cpp")
file(GLOB_RECURSE TEST_HEADERS "include/WasabiTest/*")
file(GLOB_RECURSE TEST_GLSL "src/WasabiTest/*.glsl")

# Wasabi test application(s)
assign_source_group(${TEST_SOURCES} ${TEST_HEADERS})
build_glsl(TARGET test-glsl-code VULKAN ${VULKAN_SDK_PATH} CONVERTER ${CMAKE_SOURCE_DIR}/src/spv-converter.py SOURCES ${TEST_GLSL})
if (WIN32)
    add_executable(wasabi_test WIN32 ${TEST_SOURCES} ${TEST_HEADERS})
else()
    add_executable(wasabi_test ${TEST_SOURCES} ${TEST_HEADERS})
endif()
set_property(TARGET wasabi_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
target_include_directories(wasabi_test PRIVATE "include/")
target_include_directories(wasabi_test PRIVATE "include/WasabiTest")
add_dependencies(wasabi_test test-glsl-code build-dist)
link_target_to_wasabi(wasabi_test "${CMAKE_BINARY_DIR}/dist")
enable_all_warnings(wasabi_test)

#
# Compiler-specific warnings
#

if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
    # ignore MSVC warnings for external headers
    target_compile_options(standalone-wasabi PRIVATE /experimental:external /external:W0 /external:I${STB_DIR})
    target_compile_options(wasabi_test PRIVATE /experimental:external /external:W0 /external:I${STB_DIR})
    target_compile_options(standalone-wasabi PRIVATE /experimental:external /external:W0 /external:I${TFD_DIR})
    target_compile_options(wasabi_test PRIVATE /experimental:external /external:W0 /external:I${TFD_DIR})
    target_compile_options(standalone-wasabi PRIVATE /experimental:external /external:W0 /external:I${BULLET_DIR}/src)
    target_compile_options(wasabi_test PRIVATE /experimental:external /external:W0 /external:I${BULLET_DIR}/src)
endif()

# ignore "object has no symbol" linker errors
if (MACOSX)
    set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()
