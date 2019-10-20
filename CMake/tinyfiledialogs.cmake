
function(build_tinyfiledialogs TFD_DIR_VAR DEPENDENCIES_DIR)
    set(${TFD_DIR_VAR} ${DEPENDENCIES_DIR}/tinyfiledialogs)
    set(${TFD_DIR_VAR} ${DEPENDENCIES_DIR}/tinyfiledialogs PARENT_SCOPE)

    if(NOT EXISTS "${${TFD_DIR_VAR}}/tinyfiledialogs.h")
        message(STATUS "${${TFD_DIR_VAR}}/tinyfiledialogs.h was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()

    file(GLOB_RECURSE TFD_SOURCES "${TFD_DIR}/tinyfiledialogs.c")
    add_library(tinyfiledialogs STATIC ${TFD_SOURCES})
    prepare_dependency(NAME "TinyFileDialogs" TARGETS tinyfiledialogs)
endfunction()