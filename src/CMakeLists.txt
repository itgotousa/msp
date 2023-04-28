# cmake -G "NMake Makefiles" D:\code\msp\src
# cmake -G "NMake Makefiles" D:\code\msp\src -DCMAKE_BUILD_TYPE=Debug -DCMAKE_GENERATOR_PLATFORM=x64
cmake_minimum_required(VERSION 3.26)

project(msp)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3 /MTd")

# add_executable(msp main.cpp hashfn.c aset.c mcxt.c)
add_executable(msp main.cpp mcxt.c aset.c globals.c pg_leftmost_one_pos.c)
