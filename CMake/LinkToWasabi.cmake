
function (link_target_to_wasabi target_name wasabi_path)
    # find vulkan
    find_package(Vulkan)
        if (NOT Vulkan_FOUND)
            message(FATAL_ERROR "Could not find Vulkan library!")
    endif()

    # link to vulkan
    target_include_directories(${target_name} PRIVATE SYSTEM ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(${target_name} ${Vulkan_LIBRARY})

    # link to wasabi
    target_include_directories(${target_name} PRIVATE ${wasabi_path}/include)
    if (MSVC)
        target_link_libraries(${target_name} debug ${wasabi_path}/lib/Debug/libwasabi.a)
        target_link_libraries(${target_name} optimized ${wasabi_path}/lib/Release/libwasabi.a)
    else()
        target_link_libraries(${target_name} ${wasabi_path}/lib/libwasabi.a)
    endif()

    # set MacOS frameworks
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(wasabi_test "-framework Cocoa" "-framework CoreAudio" "-framework IOKit" "-framework CoreFoundation" "-framework CoreVideo" "-framework AudioUnit")
    endif()
endfunction()
