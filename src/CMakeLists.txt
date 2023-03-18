add_executable(${CMAKE_PROJECT_NAME} Main.cpp Process.cpp Video.cpp ${VERSION_FILE})

if (UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT} ${LIBSWRESAMPLE})
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT} ${LIBSWRESAMPLE})
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

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)
