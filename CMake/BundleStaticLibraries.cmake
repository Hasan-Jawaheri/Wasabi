
function(bundle_libraries_old)
cmake_parse_arguments(
    PARSED_ARGS # prefix of output variables
    "" # list of names of the boolean arguments (only defined ones will be true)
    "TARGET_NAME" # list of names of mono-valued arguments
    "LIBRARIES" # list of names of multi-valued arguments (output variables are lists)
    ${ARGN} # arguments of the function to parse, here we take the all original ones
)

set(TARGET_NAME ${PARSED_ARGS_TARGET_NAME})
set(STANDALONE_LIB_NAME "$<TARGET_FILE_DIR:${TARGET_NAME}>/standalone-$<TARGET_FILE_NAME:${TARGET_NAME}>")
set(LIBRARIES ${PARSED_ARGS_LIBRARIES} ${STANDALONE_LIB_NAME})

if (MSVC)
    # use MSVC lib tool (bundled with MSVC)
    set(MSVC_LIBTOOL "${CMAKE_CXX_COMPILER}/../lib.exe")
    set(BUNDLE_COMMAND ${MSVC_LIBTOOL} "/OUT:$<TARGET_FILE:${TARGET_NAME}>" ${LIBRARIES})
# elseif(MACOSX)
#     # use libtool
#     set(BUNDLE_COMMAND "libtool" "-static" "-a" "-o" "$<TARGET_FILE:${TARGET_NAME}>" ${LIBRARIES})
else()
    # use AR
    set(BUNDLE_COMMAND ${CMAKE_AR}  "cru" "$<TARGET_FILE:${TARGET_NAME}>" ${LIBRARIES})# "&&" "ranlib" "$<TARGET_FILE:${TARGET_NAME}>")
endif()

add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:${TARGET_NAME}>
)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:${TARGET_NAME}> ${STANDALONE_LIB_NAME}
    COMMAND ${BUNDLE_COMMAND}
)
endfunction()

function(bundle_static_library tgt_name bundled_tgt_name)
    list(APPEND static_libs ${tgt_name})

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

    _recursively_collect_dependencies(${tgt_name})

    list(REMOVE_DUPLICATES static_libs)

    set(bundled_tgt_full_name 
    ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

    if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$" OR MACOSX)
        if (MACOSX)
            set(AR_COMMAND "libtool" "-no_warning_for_no_symbols" "-static" "-o")
        else()
            set(AR_COMMAND ${CMAKE_AR} rcs)
            if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
                set(AR_COMMAND ${CMAKE_CXX_COMPILER_AR} rcs)
            endif()
        endif()

        foreach(tgt IN LISTS static_libs)
            list(APPEND static_libs_full_names "$<TARGET_FILE:${tgt}>")
        endforeach()

        # message(ERROR_FATAL )
        add_custom_command(
            COMMAND ${AR_COMMAND} ${bundled_tgt_full_name} ${static_libs_full_names}
            OUTPUT ${bundled_tgt_full_name}
            COMMENT "Bundling ${bundled_tgt_name}"
            VERBATIM)
    elseif(MSVC)
        find_program(lib_tool lib)

        foreach(tgt IN LISTS static_libs)
            list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
        endforeach()

        add_custom_command(
            COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
            OUTPUT ${bundled_tgt_full_name}
            COMMENT "Bundling ${bundled_tgt_name}"
            VERBATIM)
    else()
        message(FATAL_ERROR "Unknown bundle scenario!")
    endif()

    add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
    add_dependencies(bundling_target ${tgt_name})

    add_library(${bundled_tgt_name} STATIC IMPORTED)
    set_target_properties(${bundled_tgt_name} 
    PROPERTIES 
        IMPORTED_LOCATION ${bundled_tgt_full_name}
        INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)
    add_dependencies(${bundled_tgt_name} bundling_target)

endfunction()
