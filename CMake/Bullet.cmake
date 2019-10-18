
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
    if (MSVC)
        target_compile_options(Bullet3Collision PRIVATE /W0)
        target_compile_options(Bullet3Common PRIVATE /W0)
        target_compile_options(Bullet3Dynamics PRIVATE /W0)
        target_compile_options(Bullet3Geometry PRIVATE /W0)
        target_compile_options(BulletCollision PRIVATE /W0)
        target_compile_options(BulletDynamics PRIVATE /W0)
        target_compile_options(LinearMath PRIVATE /W0)
        target_compile_options(BulletInverseDynamics PRIVATE /W0)
        target_compile_options(BulletSoftBody PRIVATE /W0)
    else()
        target_compile_options(Bullet3Collision PRIVATE -w)
        target_compile_options(Bullet3Common PRIVATE -w)
        target_compile_options(Bullet3Dynamics PRIVATE -w)
        target_compile_options(Bullet3Geometry PRIVATE -w)
        target_compile_options(BulletCollision PRIVATE -w)
        target_compile_options(BulletDynamics PRIVATE -w)
        target_compile_options(LinearMath PRIVATE -w)
        target_compile_options(BulletInverseDynamics PRIVATE -w)
        target_compile_options(BulletSoftBody PRIVATE -w)
    endif()
endfunction()