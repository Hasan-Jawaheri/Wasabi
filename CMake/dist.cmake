
function(build_dist TARGET_NAME WASABI_TARGET_NAME)
    get_property(OUTPUT_LIBPATH TARGET ${WASABI_TARGET_NAME} PROPERTY LOCATION)
    get_filename_component(OUTPUT_LIBNAME ${OUTPUT_LIBPATH} NAME)
    if (MSVC)
        add_custom_command(
            OUTPUT "dist/lib/Debug/${OUTPUT_LIBNAME}" "dist/lib/Release/${OUTPUT_LIBNAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include/Wasabi/" "${CMAKE_BINARY_DIR}/dist/include/Wasabi/"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${WASABI_TARGET_NAME}> "$<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/dist/lib/Debug/$<TARGET_FILE_NAME:${WASABI_TARGET_NAME}>>$<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/dist/lib/Release/$<TARGET_FILE_NAME:${WASABI_TARGET_NAME}>>"
            COMMENT "Building the dist/ folder"
            DEPENDS "$<TARGET_FILE:${WASABI_TARGET_NAME}>"
        )
        add_custom_target(${TARGET_NAME}
            DEPENDS "${CMAKE_BINARY_DIR}/dist/lib/Debug/${OUTPUT_LIBNAME}" "${CMAKE_BINARY_DIR}/dist/lib/Release/${OUTPUT_LIBNAME}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT "dist/lib/${OUTPUT_LIBNAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include/Wasabi/" "${CMAKE_BINARY_DIR}/dist/include/Wasabi/"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${WASABI_TARGET_NAME}> "${CMAKE_BINARY_DIR}/dist/lib/$<TARGET_FILE_NAME:${WASABI_TARGET_NAME}>"
            COMMENT "Building the dist/ folder"
            DEPENDS "$<TARGET_FILE:${WASABI_TARGET_NAME}>"
        )
        add_custom_target(${TARGET_NAME}
            DEPENDS "dist/lib/${OUTPUT_LIBNAME}"
            VERBATIM
        )
    endif()
    add_dependencies(${TARGET_NAME} ${WASABI_TARGET_NAME})
endfunction()
