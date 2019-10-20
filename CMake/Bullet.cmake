
function(build_bullet BULLET_DIR_VAR DEPENDENCIES_DIR)
    set(${BULLET_DIR_VAR} ${DEPENDENCIES_DIR}/bullet3)
    set(${BULLET_DIR_VAR} ${DEPENDENCIES_DIR}/bullet3 PARENT_SCOPE)

    if(NOT EXISTS "${${BULLET_DIR_VAR}}/CMakeLists.txt")
        message(STATUS "${${BULLET_DIR_VAR}}/CMakeLists.txt was not found")
        message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()

    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
    add_subdirectory("${${BULLET_DIR_VAR}}")
    prepare_dependency(NAME "Bullet" TARGETS Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry BulletCollision BulletDynamics LinearMath BulletInverseDynamics BulletSoftBody Bullet3OpenCL_clew Bullet2FileLoader)
endfunction()
