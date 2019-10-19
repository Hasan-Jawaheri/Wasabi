cmake_minimum_required(VERSION 3.15)

project(Wasabi)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)

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

set(DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
initialize_submodules()
build_open_al("OPENAL_DIR" "${DEPENDENCIES_DIR}")
build_bullet("BULLET_DIR" "${DEPENDENCIES_DIR}")
build_glfw("GLFW_DIR" "${DEPENDENCIES_DIR}")
build_stb("STB_DIR" "${DEPENDENCIES_DIR}")
build_tinyfiledialogs("TFD_DIR" "${DEPENDENCIES_DIR}")

# Source files
file(GLOB_RECURSE TFD_SOURCES "${TFD_DIR}/tinyfiledialogs.c")

file(GLOB_RECURSE LIB_SOURCES "src/Wasabi/*.cpp")
file(GLOB_RECURSE LIB_GLSL "src/Wasabi/*.glsl")
file(GLOB_RECURSE LIB_HEADERS "include/Wasabi/*")

file(GLOB_RECURSE TEST_SOURCES "src/WasabiTest/*.cpp")
file(GLOB_RECURSE TEST_HEADERS "include/WasabiTest/*")
file(GLOB_RECURSE TEST_GLSL "src/WasabiTest/*.glsl")

file(GLOB_RECURSE FBX_LOADER_SOURCES "src/FBXLoader/*.cpp")
file(GLOB_RECURSE FBX_LOADER_HEADERS "include/FBXLoader/*")

# Wasabi library
assign_source_group(${LIB_SOURCES} ${TFD_SOURCES} ${LIB_GLSL} ${LIB_HEADERS})
build_glsl(TARGET lib-glsl-code VULKAN ${VULKAN_SDK_PATH} SOURCES ${LIB_GLSL})
add_library(standalone-wasabi STATIC ${LIB_SOURCES} ${TFD_SOURCES} ${LIB_GLSL} ${LIB_HEADERS})
add_dependencies(standalone-wasabi lib-glsl-code)
target_compile_definitions(standalone-wasabi PRIVATE COMPILING_WASABI)
target_compile_definitions(standalone-wasabi PRIVATE AL_LIBTYPE_STATIC) # <-- this makes the OpenAL headers not do dllimport (on Windows)
target_include_directories(standalone-wasabi PRIVATE "include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM ${Vulkan_INCLUDE_DIRS})
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${OPENAL_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${BULLET_DIR}/src/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${GLFW_DIR}/include/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${STB_DIR}/")
target_include_directories(standalone-wasabi PRIVATE SYSTEM "${TFD_DIR}/")
enable_all_warnings(standalone-wasabi)

# Ignore TFD warnings
if(MSVC)
    set_source_files_properties(${TFD_SOURCES} PROPERTIES COMPILE_FLAGS /wd4267)
    set_source_files_properties(${STB_DIR}/stb.h PROPERTIES COMPILE_FLAGS /wd4701)
elseif(MACOSX)
else()
    set_source_files_properties(${TFD_SOURCES} PROPERTIES COMPILE_FLAGS -Wno-4267)
    set_source_files_properties(${STB_DIR}/stb.h PROPERTIES COMPILE_FLAGS -Wno-4701)
endif()

# Bundle the libraries together
bundle_static_library(
    TARGET standalone-wasabi
    BUNDLED_TARGET wasabi
    DEPENDENCIES ex-common OpenAL glfw Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry BulletCollision BulletDynamics LinearMath)
# Build the dist folder
build_dist(build-dist wasabi)

# Wasabi test application(s)
assign_source_group(${TEST_SOURCES} ${TEST_HEADERS})
build_glsl(TARGET test-glsl-code VULKAN ${VULKAN_SDK_PATH} SOURCES ${TEST_GLSL})
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

# FBX Loader
if (FBXSDK_ROOT)
    assign_source_group(${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    if (WIN32)
        add_executable(fbx_loader WIN32 ${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    else()
        add_executable(fbx_loader ${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    endif()
    target_include_directories(fbx_loader PRIVATE "include/")
    target_include_directories(fbx_loader PRIVATE "include/FBXLoader")
    link_target_to_wasabi(fbx_loader "${CMAKE_BINARY_DIR}/dist")
    target_link_libraries(fbx_loader optimized ${FBX_LIBRARIES_RELEASE})
    target_link_libraries(fbx_loader debug ${FBX_LIBRARIES_DEBUG})
    target_include_directories(fbx_loader PRIVATE ${FBX_INCLUDE_DIR})
    set_property(TARGET fbx_loader PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    enable_all_warnings(fbx_loader)
endif()

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
