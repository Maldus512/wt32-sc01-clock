# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_definitions(-DLV_HOR_RES_MAX=480)
add_definitions(-DLV_VER_RES_MAX=320)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wt32sc01-clock)

target_add_binary_data(wt32sc01-clock.elf "webapp/index.html" TEXT)
