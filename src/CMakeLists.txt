cmake_minimum_required(VERSION 3.26)

project(msp)

add_subdirectory(pgcore)
add_subdirectory(md)
add_subdirectory(font)
add_subdirectory(svg)
add_subdirectory(png)
add_subdirectory(gif)
add_subdirectory(arch/windows)

# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3 /MTd")

# add_executable(msp main.cpp hashfn.c aset.c mcxt.c)


