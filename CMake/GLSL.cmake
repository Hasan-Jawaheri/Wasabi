
# adds a custom target that compiles the GLSL code using the compile-glsl-code.py python script
function(build_glsl TARGET_NAME VULKAN_SDK_PATH)
    # Make sure python is installed
    find_package(Python3 COMPONENTS Interpreter)
    if (NOT Python3_FOUND)
        message(FATAL_ERROR "Could not find python3!")
    endif()

    add_custom_target(${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/compile-glsl-code.py ${VULKAN_SDK_PATH} ${CMAKE_SOURCE_DIR}/src/
        VERBATIM
    )
endfunction()
