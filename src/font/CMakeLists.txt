project(libfont)

file(GLOB LIBFONT_SRC "*.c")

# add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${LIBFONT_SRC})
#message("==X===> CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${PROJECT_SOURCE_DIR})

