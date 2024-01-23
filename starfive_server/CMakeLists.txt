cmake_minimum_required(VERSION 3.6)

project(starfive-server)
add_executable(starfive-server comm.cpp starfive_server.cpp tuning_base.cpp tuning_main.cpp broadcast_main.cpp param_convert.cpp)
target_link_libraries(starfive-server libcamera_app)

set(EXECUTABLES starfive-server)

set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
install(TARGETS ${EXECUTABLES} RUNTIME DESTINATION bin)
