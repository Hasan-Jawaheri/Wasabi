
function(build_stb STB_DIR_VAR DEPENDENCIES_DIR)
    set(${STB_DIR_VAR} ${DEPENDENCIES_DIR}/stb)
    set(${STB_DIR_VAR} ${DEPENDENCIES_DIR}/stb PARENT_SCOPE)

    if(NOT EXISTS "${${STB_DIR_VAR}}/stb.h")
        message(STATUS "${${STB_DIR_VAR}}/stb.h was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()
endfunction()