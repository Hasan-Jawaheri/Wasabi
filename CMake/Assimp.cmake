
function(build_assimp ASSIMP_DIR_VAR DEPENDENCIES_DIR)
    set(${ASSIMP_DIR_VAR} ${DEPENDENCIES_DIR}/assimp)
    set(${ASSIMP_DIR_VAR} ${DEPENDENCIES_DIR}/assimp PARENT_SCOPE)

    if(NOT EXISTS "${${ASSIMP_DIR_VAR}}/CMakeLists.txt")
        message(STATUS "${${ASSIMP_DIR_VAR}}/CMakeLists.txt was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()

    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_OPT_BUILD_PACKAGES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${${ASSIMP_DIR_VAR}}")
    prepare_dependency(NAME "Assimp" TARGETS assimp zlib zlibstatic IrrXML UpdateAssimpLibsDebugSymbolsAndDLLs)
endfunction()
