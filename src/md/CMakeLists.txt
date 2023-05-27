project(libmd)

file(GLOB LIBMD_SRC "*.c")

# add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${LIBMD_SRC})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR} ${PROJECT_SOURCE_DIR})

