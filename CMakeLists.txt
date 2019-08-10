cmake_minimum_required(VERSION 3.15)

project(Wasabi)
set(CMAKE_CXX_STANDARD 17)

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
if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else()
	message(STATUS "Vulkan library: " ${Vulkan_LIBRARY})
	message(STATUS "Vulkan include: " ${Vulkan_INCLUDE_DIRS})
endif()
link_libraries(${Vulkan_LIBRARY})
include_directories(SYSTEM ${Vulkan_INCLUDE_DIRS})
#####################################

############# OpenAL ################
find_package(OpenAL)
if (NOT OPENAL_FOUND)
	message(FATAL_ERROR "Could not find OpenAL library!")
else()
	message(STATUS "OpenAL library: " ${OPENAL_LIBRARY})
	message(STATUS "OpenAL include: " ${OPENAL_INCLUDE_DIR})
endif()
link_libraries(${OPENAL_LIBRARY})
include_directories(SYSTEM ${OPENAL_INCLUDE_DIR})
#####################################

############# Bullet ################
find_package(Bullet)
if (NOT Bullet_FOUND)
	message(FATAL_ERROR "Could not find Bullet physics library!")
else()
	message(STATUS "Bullet libraries: " ${BULLET_LIBRARIES})
    message(STATUS "Bullet include: " ${BULLET_INCLUDE_DIRS})
endif()
link_libraries(${BULLET_LIBRARIES})
include_directories(SYSTEM ${BULLET_INCLUDE_DIRS})
#####################################

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

# put the Wasabi include files in the build directory
file(COPY "include/Wasabi/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/include/Wasabi/")

# External libraries
include_directories(SYSTEM "lib/glfw-3.3/include/")
include_directories(SYSTEM "lib/stb/include/")

include_directories("include/")

# Wasabi library
assign_source_group(${LIB_SOURCES} ${LIB_HEADERS})
add_library(wasabi STATIC ${LIB_SOURCES} ${LIB_HEADERS})
add_custom_command(TARGET wasabi
    PRE_BUILD
    COMMAND cd ${CMAKE_SOURCE_DIR}/src/ && python compile-glsl-code.py
)

# Wasabi test application(s)
assign_source_group(${TEST_SOURCES} ${TEST_HEADERS})
if (WIN32)
    add_executable(wasabi_test WIN32 ${TEST_SOURCES} ${TEST_HEADERS})
else()
    add_executable(wasabi_test ${TEST_SOURCES} ${TEST_HEADERS})
endif()
target_include_directories(wasabi_test PRIVATE "include/WasabiTest")
target_link_libraries(wasabi_test wasabi)
target_link_libraries(wasabi_test glfw)
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
    target_link_libraries(fbx_loader wasabi)
    target_link_libraries(fbx_loader optimized ${FBX_LIBRARIES_RELEASE})
    target_link_libraries(fbx_loader debug ${FBX_LIBRARIES_DEBUG})
    target_link_libraries(fbx_loader glfw)
    target_include_directories(fbx_loader PRIVATE ${FBX_INCLUDE_DIR})
    set_property(TARGET fbx_loader PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# ignore MSVC warnings for external headers
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0 ")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
    
    get_filename_component(STB_INCLUDE_PATH "lib/stb/include/" ABSOLUTE)
    target_compile_options(wasabi PRIVATE /external:I${STB_INCLUDE_PATH})
    target_compile_options(wasabi_test PRIVATE /external:I${STB_INCLUDE_PATH})
    target_compile_options(wasabi PRIVATE /external:I${BULLET_INCLUDE_DIRS})
    target_compile_options(wasabi_test PRIVATE /external:I${BULLET_INCLUDE_DIRS})
endif()
