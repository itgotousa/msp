project(libfont)

file(GLOB LIBFONT_SRC "*.c")

# add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${LIBFONT_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

