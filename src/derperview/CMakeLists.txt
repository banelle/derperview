add_executable(${CMAKE_PROJECT_NAME} Main.cpp ${VERSION_FILE})

if (UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads lib${CMAKE_PROJECT_NAME})
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} lib${CMAKE_PROJECT_NAME})
endif()

if(WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avcodec-59.dll"
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avformat-59.dll"
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avutil-57.dll"
            "${DERPERVIEW_FFMPEG_DLL_PATH}/swresample-4.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
    )
endif()
