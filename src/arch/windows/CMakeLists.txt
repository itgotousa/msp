# the build for Windows X64
project(msp-win64)

add_executable(${PROJECT_NAME} WIN32 msp.cpp stdafx.cpp threadrender.cpp TextRender.cpp msp.rc)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/wtl ${CMAKE_SOURCE_DIR} ${LIBPNG_SOURCE_DIR} ${LIBSVG_SOURCE_DIR} ${LIBGIF_SOURCE_DIR} ${pgcore_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PUBLIC pgcore libsvg libgif libpng)
