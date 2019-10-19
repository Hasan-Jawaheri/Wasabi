
# adds a custom target that compiles the GLSL code using the compile-glsl-code.py python script
function(build_glsl)
    set(optionArgs "")
    set(oneValueArgs "TARGET" "VULKAN")
    set(multiValueArgs "SOURCES")
    cmake_parse_arguments(FUNCTION_ARGS "${optionArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Make sure python is installed
    find_package(Python3 COMPONENTS Interpreter)
    if (NOT Python3_FOUND)
        message(FATAL_ERROR "Could not find python3!")
    endif()

    if(WIN32)
        set(glslangValidator ${FUNCTION_ARGS_VULKAN}/Bin/glslangValidator.exe)
    else()
        set(glslangValidator ${FUNCTION_ARGS_VULKAN}/bin/glslangValidator)
    endif()
    set(spvConverter ${CMAKE_SOURCE_DIR}/src/spv-converter.py)
    set(entryPoint "main")
    set(GLSL_COMPILED_SOURCES "")
    foreach(file IN LISTS FUNCTION_ARGS_SOURCES)
        if(file MATCHES ".*.vert.glsl$" OR file MATCHES ".*.frag.glsl$" OR file MATCHES ".*.geom.glsl$" OR file MATCHES ".*.tesc.glsl$" OR file MATCHES ".*.tese.glsl$" OR file MATCHES ".*.comp.glsl$")
            set(tmpFile ${file}.tmp.spv)
            set(outputFile ${file}.spv)
            list(APPEND GLSL_COMPILED_SOURCES ${outputFile})
            add_custom_command(
                OUTPUT ${outputFile}
                COMMAND ${glslangValidator} "-V" "--entry-point" "${entryPoint}" "${file}" "-o" "${tmpFile}"
                COMMAND ${Python3_EXECUTABLE} ${spvConverter} "${tmpFile}" "${outputFile}"
                COMMAND ${CMAKE_COMMAND} -E remove -f ${tmpFile}
                SOURCES ${file}
                VERBATIM
            )
        endif()
    endforeach()

    add_custom_target(${FUNCTION_ARGS_TARGET}
        DEPENDS ${GLSL_COMPILED_SOURCES}
        VERBATIM
    )
endfunction()
