
function(build_boxer BOXER_DIR_VAR DEPENDENCIES_DIR)
    set(${BOXER_DIR_VAR} ${DEPENDENCIES_DIR}/Boxer)
    set(${BOXER_DIR_VAR} ${DEPENDENCIES_DIR}/Boxer PARENT_SCOPE)

    if(NOT EXISTS "${${BOXER_DIR_VAR}}/CMakeLists.txt")
        message(STATUS "${${BOXER_DIR_VAR}}/CMakeLists.txt was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()

    add_subdirectory("${${BOXER_DIR_VAR}}")
endfunction()
