
function (link_target_to_wasabi target_name wasabi_path)
# find vulkan
find_package(Vulkan)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
endif()
set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS} PARENT_SCOPE)
set(Vulkan_LIBRARY ${Vulkan_LIBRARY} PARENT_SCOPE)
set(VULKAN_SDK_PATH ${Vulkan_INCLUDE_DIRS}/.. PARENT_SCOPE)

# link to wasabi
target_include_directories(${target_name} PRIVATE ${wasabi_path}/include)
message(STATUS "Wasabi include: " ${wasabi_path}/include)
if (MSVC)
    target_link_libraries(${target_name} debug ${wasabi_path}/lib/Debug/wasabi.lib)
    target_link_libraries(${target_name} optimized ${wasabi_path}/lib/Release/wasabi.lib)
    message(STATUS "Wasabi library: " ${wasabi_path}/lib/<Release/Debug>/wasabi.lib)

    target_link_libraries(${target_name} "winmm.lib")
else()
    target_link_libraries(${target_name} ${wasabi_path}/lib/libwasabi.a)
    message(STATUS "Wasabi library: " ${wasabi_path}/lib/libwasabi.a)

    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        # set MacOS frameworks
        target_link_libraries(${target_name} "-framework Cocoa" "-framework CoreAudio" "-framework IOKit" "-framework CoreFoundation" "-framework CoreVideo" "-framework AudioUnit")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        # set Linux dependencies
        target_link_libraries(${target_name} -lpthread -lX11 -ldl -lstdc++fs -lz)
    endif()
endif()

# link to vulkan
target_include_directories(${target_name} PRIVATE SYSTEM ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${target_name} ${Vulkan_LIBRARY})
endfunction()
