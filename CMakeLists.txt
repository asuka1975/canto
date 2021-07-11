cmake_minimum_required(VERSION 3.19)
project(canto)

find_package(Freetype REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_library(canto include/canto/ft_wrapper.h src/ft_wrapper.cpp)
target_include_directories(canto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${FREETYPE_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/glplusplus/include)

add_subdirectory(glplusplus)