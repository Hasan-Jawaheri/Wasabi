
function(build_glfw GLFW_DIR_VAR DEPENDENCIES_DIR)
    set(${GLFW_DIR_VAR} ${DEPENDENCIES_DIR}/glfw)
    set(${GLFW_DIR_VAR} ${DEPENDENCIES_DIR}/glfw PARENT_SCOPE)

    if(NOT EXISTS "${${GLFW_DIR_VAR}}/CMakeLists.txt")
        message(STATUS "${${GLFW_DIR_VAR}}/CMakeLists.txt was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${${GLFW_DIR_VAR}}")
endfunction()
