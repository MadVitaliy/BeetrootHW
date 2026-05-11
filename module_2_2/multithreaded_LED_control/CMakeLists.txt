cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(multithreaded_led_control  VERSION 1.0 LANGUAGES CXX)
