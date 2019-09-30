
# adds a custom target that compiles the GLSL code using the compile-glsl-code.py python script
function(build_glsl TARGET_NAME VULKAN_SDK_PATH)
    add_custom_target(${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/
        COMMAND python3 compile-glsl-code.py ${VULKAN_SDK_PATH}
    )
endfunction()
