cmake_minimum_required(VERSION 3.15)

project(Wasabi)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/CMake")
include(WasabiHelpers)

# Find the vulkan installation
find_package(Vulkan)
if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else()
	message(STATUS "Vulkan library: " ${Vulkan_LIBRARY})
	message(STATUS "Vulkan include: " ${Vulkan_INCLUDE_DIRS})
    set(VULKAN_SDK_PATH ${Vulkan_INCLUDE_DIRS}/..)
endif()

set(DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
initialize_submodules()
build_open_al("OPENAL_DIR" "${DEPENDENCIES_DIR}")
build_bullet("BULLET_DIR" "${DEPENDENCIES_DIR}")
build_glfw("GLFW_DIR" "${DEPENDENCIES_DIR}")
build_stb("STB_DIR" "${DEPENDENCIES_DIR}")
build_tinyfiledialogs("TFD_DIR" "${DEPENDENCIES_DIR}")
build_glsl(glsl-code ${VULKAN_SDK_PATH})

############# FBX SDK ###############
if (NOT FBXSDK_ROOT)
    message(WARNING "Could not find FBX SDK")
else()
    SET(FBX_LIBRARIES_RELEASE "${FBXSDK_ROOT}/lib/vs2015/x64/release/libfbxsdk-md.lib")
    SET(FBX_LIBRARIES_DEBUG "${FBXSDK_ROOT}/lib/vs2015/x64/debug/libfbxsdk-md.lib")
    SET(FBX_INCLUDE_DIR ${FBXSDK_ROOT}/include)

	message(STATUS "FBXSDK libraries: " ${FBX_LIBRARIES_RELEASE})
	message(STATUS "FBXSDK include: " ${FBX_INCLUDE_DIR})
endif()
#####################################

# Source files
file(GLOB_RECURSE LIB_SOURCES "src/Wasabi/*.cpp" "${TFD_DIR}/tinyfiledialogs.c")
file(GLOB_RECURSE LIB_GLSL "src/Wasabi/*.glsl")
file(GLOB_RECURSE LIB_HEADERS "include/Wasabi/*")
file(GLOB_RECURSE TEST_SOURCES "src/WasabiTest/*.cpp")
file(GLOB_RECURSE TEST_HEADERS "include/WasabiTest/*")
file(GLOB_RECURSE FBX_LOADER_SOURCES "src/FBXLoader/*.cpp")
file(GLOB_RECURSE FBX_LOADER_HEADERS "include/FBXLoader/*")

include_directories("include/")

# Wasabi library
assign_source_group(${LIB_SOURCES} ${LIB_GLSL} ${LIB_HEADERS})
add_library(standalone-wasabi STATIC ${LIB_SOURCES} ${LIB_GLSL} ${LIB_HEADERS})
add_dependencies(standalone-wasabi glsl-code)
target_compile_definitions(standalone-wasabi PRIVATE COMPILING_WASABI)
target_include_directories(standalone-wasabi PRIVATE SYSTEM ${Vulkan_INCLUDE_DIRS})
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${OPENAL_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${BULLET_DIR}/src/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${GLFW_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${STB_DIR}/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${TFD_DIR}/")
target_link_libraries(standalone-wasabi ${Vulkan_LIBRARY})
target_link_libraries(standalone-wasabi common ex-common OpenAL)
target_link_libraries(standalone-wasabi glfw)
target_link_libraries(standalone-wasabi Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry BulletCollision BulletDynamics LinearMath)

# Bundle the libraries together
bundle_static_library(standalone-wasabi wasabi)
# Build the dist folder
build_dist(build-dist wasabi)

# Wasabi test application(s)
assign_source_group(${TEST_SOURCES} ${TEST_HEADERS})
if (WIN32)
    add_executable(wasabi_test WIN32 ${TEST_SOURCES} ${TEST_HEADERS})
else()
    add_executable(wasabi_test ${TEST_SOURCES} ${TEST_HEADERS})
endif()
add_dependencies(wasabi_test build-dist)
target_include_directories(wasabi_test PRIVATE "include/WasabiTest")
link_target_to_wasabi(wasabi_test "${CMAKE_BINARY_DIR}/dist")
set_property(TARGET wasabi_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# FBX Loader
if (FBXSDK_ROOT)
    assign_source_group(${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    if (WIN32)
        add_executable(fbx_loader WIN32 ${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    else()
        add_executable(fbx_loader ${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    endif()
    target_include_directories(fbx_loader PRIVATE "include/FBXLoader")
    link_target_to_wasabi(fbx_loader "${CMAKE_BINARY_DIR}/dist")
    target_link_libraries(fbx_loader optimized ${FBX_LIBRARIES_RELEASE})
    target_link_libraries(fbx_loader debug ${FBX_LIBRARIES_DEBUG})
    target_include_directories(fbx_loader PRIVATE ${FBX_INCLUDE_DIR})
    set_property(TARGET fbx_loader PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# ignore MSVC warnings for external headers
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")

    target_compile_options(standalone-wasabi PRIVATE /external:I${STB_DIR})
    target_compile_options(wasabi_test PRIVATE /external:I${STB_DIR})
    target_compile_options(standalone-wasabi PRIVATE /external:I${BULLET_DIR}/src)
    target_compile_options(wasabi_test PRIVATE /external:I${BULLET_DIR}/src)
endif()

# ignore "object has no symbol" linker errors
if (MACOSX)
    set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> Scr <TARGET> <LINK_FLAGS> <OBJECTS>")
    set(CMAKE_C_ARCHIVE_FINISH   "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
    set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_RANLIB> -no_warning_for_no_symbols -c <TARGET>")
endif()
