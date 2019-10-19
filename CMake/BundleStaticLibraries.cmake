
function(bundle_static_library)
    set(optionArgs "")
    set(oneValueArgs TARGET BUNDLED_TARGET)
    set(multiValueArgs DEPENDENCIES)
    cmake_parse_arguments(FUNCTION_ARGS "${optionArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(tgt_name ${FUNCTION_ARGS_TARGET})
    set(bundled_tgt_name ${FUNCTION_ARGS_BUNDLED_TARGET})

    function(_recursively_collect_dependencies input_target)
        set(_input_link_libraries LINK_LIBRARIES)
        get_target_property(_input_type ${input_target} TYPE)
        if (${_input_type} STREQUAL "INTERFACE_LIBRARY")
            set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
        endif()
        get_target_property(public_dependencies ${input_target} ${_input_link_libraries})
        foreach(dependency IN LISTS public_dependencies)
            if(TARGET ${dependency})
            get_target_property(alias ${dependency} ALIASED_TARGET)
            if (TARGET ${alias})
                set(dependency ${alias})
            endif()
            get_target_property(_type ${dependency} TYPE)
            if (${_type} STREQUAL "STATIC_LIBRARY")
                list(APPEND static_libs ${dependency})
            endif()

            get_property(library_already_added
                GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency})
            if (NOT library_already_added)
                set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency} ON)
                _recursively_collect_dependencies(${dependency})
            endif()
            endif()
        endforeach()
        set(static_libs ${static_libs} PARENT_SCOPE)
    endfunction()

    list(APPEND static_libs ${tgt_name})
    _recursively_collect_dependencies(${tgt_name})
    list(APPEND all_static_libs ${static_libs})
    foreach(dependency IN LISTS FUNCTION_ARGS_DEPENDENCIES)
        list(APPEND static_libs ${dependency})
        _recursively_collect_dependencies(${dependency})
        list(APPEND all_static_libs ${static_libs})
    endforeach()

    list(REMOVE_DUPLICATES all_static_libs)

    set(bundled_tgt_full_name ${PROJECT_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

    if (MACOSX)
        foreach(tgt IN LISTS all_static_libs)
            list(APPEND static_libs_full_names "$<TARGET_FILE:${tgt}>")
        endforeach()

        set(BUNDLE_TOOL "libtool")
        set(LIBTOOL_COMMAND ${BUNDLE_TOOL} "-no_warning_for_no_symbols" "-static" "-o")
        set(BUNDLE_COMMAND ${LIBTOOL_COMMAND} ${bundled_tgt_full_name} ${static_libs_full_names})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
            "CREATE ${bundled_tgt_full_name}\n")

        foreach(tgt IN LISTS all_static_libs)
        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
            "ADDLIB $<TARGET_FILE:${tgt}>\n")
        endforeach()

        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
        file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

        file(GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
            INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in)

        set(ar_tool ${CMAKE_AR})
        if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
            set(ar_tool ${CMAKE_CXX_COMPILER_AR})
        endif()

        set(BUNDLE_TOOL ${ar_tool})
        set(BUNDLE_COMMAND ${BUNDLE_TOOL} -all -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar)
    elseif(MSVC)
        foreach(tgt IN LISTS all_static_libs)
            list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
        endforeach()

        if(NOT CMAKE_LIBTOOL)
            find_program(lib_tool lib HINTS "${CMAKE_CXX_COMPILER}/..")
            if("${lib_tool}" STREQUAL "lib_tool-NOTFOUND")
                message(FATAL_ERROR "Cannot locate libtool to bundle libraries")
            endif()
        else()
            set(${lib_tool} ${CMAKE_LIBTOOL})
        endif()
        set(BUNDLE_TOOL ${lib_tool})
        set(BUNDLE_COMMAND ${BUNDLE_TOOL} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names})
    else()
        message(FATAL_ERROR "Unknown bundle scenario!")
    endif()

    add_custom_command(
        TARGET ${tgt_name}
        POST_BUILD
        COMMAND ${BUNDLE_COMMAND}
        COMMENT "Bundling ${bundled_tgt_name}"
    )
    add_library(${bundled_tgt_name} STATIC IMPORTED)
    set_target_properties(${bundled_tgt_name}
        PROPERTIES
            IMPORTED_LOCATION ${bundled_tgt_full_name}
            INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)
    add_dependencies(${bundled_tgt_name} ${tgt_name})
endfunction()
