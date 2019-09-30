
function(build_dist TARGET_NAME WASABI_TARGET_NAME)
    if (MSVC)
        add_custom_target(${TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include/Wasabi/" "${CMAKE_CURRENT_BINARY_DIR}/dist/include/Wasabi/"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wasabi> "$<$<CONFIG:Debug>:${CMAKE_CURRENT_BINARY_DIR}/dist/lib/debug/$<TARGET_FILE_NAME:${WASABI_TARGET_NAME}>>$<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}/dist/lib/release/$<TARGET_FILE_NAME:${WASABI_TARGET_NAME}>>"
        )
    else()
        add_custom_target(${TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include/Wasabi/" "${CMAKE_CURRENT_BINARY_DIR}/dist/include/Wasabi/"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:wasabi> "${CMAKE_CURRENT_BINARY_DIR}/dist/lib/$<TARGET_FILE_NAME:${WASABI_TARGET_NAME}>"
        )
    endif()
    add_dependencies(${TARGET_NAME} ${WASABI_TARGET_NAME})
endfunction()
