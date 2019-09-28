cmake_minimum_required(VERSION 3.15)

project(Wasabi)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/CMake")
include(WasabiHelpers)

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

set(DEPENDENCIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
initialize_submodules()
build_open_al("OPENAL_DIR" "${DEPENDENCIES_DIR}")
build_bullet("BULLET_DIR" "${DEPENDENCIES_DIR}")
build_glfw("GLFW_DIR" "${DEPENDENCIES_DIR}")
build_stb("STB_DIR" "${DEPENDENCIES_DIR}")

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
file(GLOB_RECURSE LIB_SOURCES "src/Wasabi/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "include/Wasabi/*")
file(GLOB_RECURSE TEST_SOURCES "src/WasabiTest/*.cpp")
file(GLOB_RECURSE TEST_HEADERS "include/WasabiTest/*")
file(GLOB_RECURSE FBX_LOADER_SOURCES "src/FBXLoader/*.cpp")
file(GLOB_RECURSE FBX_LOADER_HEADERS "include/FBXLoader/*")

include_directories("include/")

# Wasabi library
assign_source_group(${LIB_SOURCES} ${LIB_HEADERS})
add_compile_definitions(COMPILING_WASABI)
add_library(wasabi STATIC ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories(wasabi PRIVATE SYSTEM "${OPENAL_DIR}/include/")
target_include_directories(wasabi PRIVATE SYSTEM "${BULLET_DIR}/src/")
target_include_directories(wasabi PRIVATE SYSTEM "${GLFW_DIR}/include/")
target_include_directories(wasabi PRIVATE SYSTEM "${STB_DIR}/")
target_link_libraries(wasabi common ex-common OpenAL)
target_link_libraries(wasabi glfw)
target_link_libraries(wasabi Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry BulletCollision BulletDynamics LinearMath)
add_custom_command(TARGET wasabi PRE_BUILD
    COMMAND cd ${CMAKE_SOURCE_DIR}/src/ && python compile-glsl-code.py
)

# Bundle the libraries together
if (MSVC)
    set(MSVC_LIBTOOL "${CMAKE_CXX_COMPILER}/../lib.exe")
    add_custom_command(TARGET wasabi PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:wasabi>
    )
    add_custom_command(TARGET wasabi POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:wasabi> $<TARGET_FILE_DIR:wasabi>/wasabi-standalone.lib
    )
    add_custom_command(TARGET wasabi POST_BUILD
        COMMAND ${MSVC_LIBTOOL} "/OUT:$<TARGET_FILE:wasabi>" $<TARGET_FILE_DIR:wasabi>/wasabi-standalone.lib $<TARGET_FILE:glfw> $<TARGET_FILE:OpenAL> $<TARGET_FILE:Bullet3Collision> $<TARGET_FILE:Bullet3Common> $<TARGET_FILE:Bullet3Dynamics> $<TARGET_FILE:Bullet3Geometry> $<TARGET_FILE:BulletCollision> $<TARGET_FILE:BulletDynamics> $<TARGET_FILE:LinearMath>
    )
else()
endif()

# Create the dist/ folder in the build/
add_custom_command(TARGET wasabi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include/Wasabi/" "${CMAKE_CURRENT_BINARY_DIR}/dist/include/Wasabi/"
)
add_custom_command(TARGET wasabi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wasabi> "$<$<CONFIG:Debug>:${CMAKE_CURRENT_BINARY_DIR}/dist/lib/debug/$<TARGET_FILE_NAME:wasabi>>$<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}/dist/lib/release/$<TARGET_FILE_NAME:wasabi>>"
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
    target_include_directories(fbx_loader PRIVATE ${FBX_INCLUDE_DIR})
    set_property(TARGET fbx_loader PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# ignore MSVC warnings for external headers
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external /external:W0")
    set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "/external:I ")
    
    target_compile_options(wasabi PRIVATE /external:I${STB_DIR})
    target_compile_options(wasabi_test PRIVATE /external:I${STB_DIR})
    target_compile_options(wasabi PRIVATE /external:I${BULLET_DIR}/src)
    target_compile_options(wasabi_test PRIVATE /external:I${BULLET_DIR}/src)
endif()
