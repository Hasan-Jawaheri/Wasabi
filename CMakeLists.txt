cmake_minimum_required(VERSION 3.15)

project(Wasabi)
set(CMAKE_CXX_STANDARD 14)

# Helper function for VS filters
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

############# Vulkan ###############
find_package(Vulkan)
IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS "Vulkan library: " ${Vulkan_LIBRARY})
	message(STATUS "Vulkan include: " ${Vulkan_INCLUDE_DIRS})
ENDIF()
link_libraries(${Vulkan_LIBRARY})
include_directories(${Vulkan_INCLUDE_DIRS})
#####################################

############# OpenAL ################
find_package(OpenAL)
IF (NOT OPENAL_FOUND)
	message(FATAL_ERROR "Could not find OpenAL library!")
ELSE()
	message(STATUS "OpenAL library: " ${OPENAL_LIBRARY})
	message(STATUS "OpenAL include: " ${OPENAL_INCLUDE_DIR})
ENDIF()
link_libraries(${OPENAL_LIBRARY})
include_directories(${OPENAL_INCLUDE_DIR})
#####################################

############# Bullet ################
find_package(Bullet)
IF (NOT Bullet_FOUND)
	message(FATAL_ERROR "Could not find Bullet physics library!")
ELSE()
    # set(BULLET_LIBRARIES optimized
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/Bullet3Common.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/Bullet3Dynamics.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/Bullet3Collision.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/Bullet3Geometry.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/BulletCollision.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/BulletDynamics.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Release/LinearMath.lib"
    # )
    # set(BULLET_LIBRARIES debug
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/Bullet3Common_debug.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/Bullet3Dynamics_debug.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/Bullet3Collision_debug.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/Bullet3Geometry_debug.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/BulletCollision_debug.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/BulletDynamics_debug.lib"
    #     "C:/Users/Hasan Al-Jawaheri/Documents/bullet3-2.88/build/lib/Debug/LinearMath_debug.lib"
    # )
	message(STATUS "Bullet libraries: " ${BULLET_LIBRARIES})
    message(STATUS "Bullet include: " ${BULLET_INCLUDE_DIRS})
ENDIF()
link_libraries(${BULLET_LIBRARIES})
include_directories(${BULLET_INCLUDE_DIRS})
#####################################

############# FBX SDK ###############
IF (NOT FBXSDK_ROOT)
    message(WARNING "Could not find FBX SDK")
ELSE()
    SET(FBX_LIBRARIES_RELEASE "${FBXSDK_ROOT}/lib/vs2015/x64/release/libfbxsdk-md.lib")
    SET(FBX_LIBRARIES_DEBUG "${FBXSDK_ROOT}/lib/vs2015/x64/debug/libfbxsdk-md.lib")
    SET(FBX_INCLUDE_DIR ${FBXSDK_ROOT}/include)

	message(STATUS "FBXSDK libraries: " ${FBX_LIBRARIES_RELEASE})
	message(STATUS "FBXSDK include: " ${FBX_INCLUDE_DIR})
ENDIF()
#####################################

# GLFW3
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("lib/glfw-3.3")

# Source files
file(GLOB_RECURSE LIB_SOURCES "src/Wasabi/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "include/Wasabi/*")
file(GLOB_RECURSE TEST_SOURCES "src/WasabiTest/*.cpp")
file(GLOB_RECURSE TEST_HEADERS "include/WasabiTest/*")
file(GLOB_RECURSE FBX_LOADER_SOURCES "src/FBXLoader/*.cpp")
file(GLOB_RECURSE FBX_LOADER_HEADERS "include/FBXLoader/*")

# External libraries
include_directories("lib/glfw-3.3/include/")
include_directories("lib/stb/include/")

# Wasabi library
assign_source_group(${LIB_SOURCES} ${LIB_HEADERS})
add_library(wasabi STATIC ${LIB_SOURCES} ${LIB_HEADERS})
include_directories("include/Wasabi")
add_custom_command(TARGET wasabi
    PRE_BUILD
    COMMAND cd ${CMAKE_SOURCE_DIR}/src/ && python compile-glsl-code.py
)

# Wasabi test application(s)
assign_source_group(${TEST_SOURCES} ${TEST_HEADERS})
add_executable(wasabi_test ${TEST_SOURCES} ${TEST_HEADERS})
target_include_directories(wasabi_test PRIVATE "include/WasabiTest")
target_link_libraries(wasabi_test wasabi)
target_link_libraries(wasabi_test glfw)
set_property(TARGET wasabi_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# FBX Loader
IF (FBXSDK_ROOT)
    assign_source_group(${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    add_executable(fbx_loader ${FBX_LOADER_SOURCES} ${FBX_LOADER_HEADERS})
    target_include_directories(fbx_loader PRIVATE "include/FBXLoader")
    target_link_libraries(fbx_loader wasabi)
    target_link_libraries(fbx_loader optimized ${FBX_LIBRARIES_RELEASE})
    target_link_libraries(fbx_loader debug ${FBX_LIBRARIES_DEBUG})
    target_link_libraries(fbx_loader glfw)
    target_include_directories(fbx_loader PRIVATE ${FBX_INCLUDE_DIR})
    set_property(TARGET fbx_loader PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
ENDIF()
