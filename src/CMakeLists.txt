add_executable(${CMAKE_PROJECT_NAME} Main.cpp Process.cpp Video.cpp Resource.rc)
target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads ${LIBAVUTIL} ${LIBAVCODEC} ${LIBAVFORMAT})

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)

