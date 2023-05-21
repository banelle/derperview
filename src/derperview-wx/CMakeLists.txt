add_executable(${CMAKE_PROJECT_NAME}-wx WIN32 Main.cpp derperview-wx.hpp ProgressDialog.cpp ProgressDialog.hpp WorkerThread.cpp WorkerThread.hpp ${VERSION_FILE})

target_include_directories(${CMAKE_PROJECT_NAME}-wx PUBLIC "${PROJECT_SOURCE_DIR}/lib/wxwidgets-windows/include" "${PROJECT_SOURCE_DIR}/lib/wxwidgets-windows/include/mswu$<$<CONFIG:Debug>:d>")

if (UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME}-wx Threads::Threads lib${CMAKE_PROJECT_NAME})
else()
    target_link_libraries(${CMAKE_PROJECT_NAME}-wx lib${CMAKE_PROJECT_NAME})
endif()

target_link_libraries(${CMAKE_PROJECT_NAME}-wx optimized ${WXWIDGET_BASE_RELEASE} optimized ${WXWIDGET_CORE_RELEASE})
target_link_libraries(${CMAKE_PROJECT_NAME}-wx debug ${WXWIDGET_BASE_DEBUG} debug ${WXWIDGET_CORE_DEBUG})

if(WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME}-wx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avcodec-59.dll"
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avformat-59.dll"
            "${DERPERVIEW_FFMPEG_DLL_PATH}/avutil-57.dll"
            "${DERPERVIEW_FFMPEG_DLL_PATH}/swresample-4.dll"
            "${DERPERVIEW_WXWIDGETS_DLL_PATH}/wxbase32u$<$<CONFIG:Debug>:d>_vc14x_x64.dll"
            "${DERPERVIEW_WXWIDGETS_DLL_PATH}/wxmsw32u$<$<CONFIG:Debug>:d>_core_vc14x_x64.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
    )
endif()
