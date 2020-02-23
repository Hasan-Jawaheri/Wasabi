
function(build_loguru LOGURU_DIR_VAR DEPENDENCIES_DIR)
    set(${LOGURU_DIR_VAR} ${DEPENDENCIES_DIR}/loguru)
    set(${LOGURU_DIR_VAR} ${DEPENDENCIES_DIR}/loguru PARENT_SCOPE)

    if(NOT EXISTS "${${LOGURU_DIR_VAR}}/loguru.hpp")
        message(STATUS "${${LOGURU_DIR_VAR}}/loguru.hpp was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()
endfunction()
