add_executable(${CMAKE_PROJECT_NAME} Main.cpp Process.cpp Video.cpp ${VERSION_FILE})

if (UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT})
else()
    target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT})
endif()

if(WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avcodec-58.dll" 
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avformat-58.dll" 
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avutil-56.dll" 
            "${DERPERVIEW_FFMPEG_DLL_PATH}/swresample-3.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
    )
endif()

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)
