cmake_minimum_required(VERSION 3.10)
project(SO2__Projekt2)

set(CMAKE_CXX_STANDARD 14)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(SO2__Projekt2
        main.cpp)
#target_link_libraries(main.cpp ncurses)
target_link_libraries(SO2__Projekt2 ${CURSES_LIBRARIES})
include_directories(/usr/include)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

