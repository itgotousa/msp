cmake_minimum_required(VERSION 3.26)

project(msp)

add_subdirectory(arch/windows)

# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3 /MTd")

# add_executable(msp main.cpp hashfn.c aset.c mcxt.c)
# add_executable(${PROJECT_NAME} WIN32 msp main.cpp msp.cpp msp.rc stdafx.cpp mcxt.c aset.c globals.c pg_leftmost_one_pos.c list.c copyfuncs.c equalfuncs.c strlcpy.c dynahash.c hashfn.c globals.c)


# cmake -G "NMake Makefiles" D:\code\msp\src
# cmake -G "NMake Makefiles" D:\code\msp\src -DCMAKE_BUILD_TYPE=Debug
# cmake -G "Visual Studio 16 2019" D:\code\msp\src -A X64
