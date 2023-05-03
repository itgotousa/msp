# the build for Windows X64
project(msp-win64)

add_executable(${PROJECT_NAME} WIN32 msp.cpp stdafx.cpp msp.rc)

target_include_directories(${PROJECT_NAME} PRIVATE  ${PROJECT_SOURCE_DIR}/wtl)

