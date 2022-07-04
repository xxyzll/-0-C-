cmake_minimum_required(VERSION 3.0.0)
project(tinyWebServer VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(tinyWebServer main.cpp web_server/web_server.cpp http/http_connect.cpp timer/timer_list.cpp)

target_link_libraries(tinyWebServer -lpthread)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
