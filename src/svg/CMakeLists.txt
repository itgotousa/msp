
project(svg)

file(GLOB SVGCORE_SRC "*.c")

# add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${SVGCORE_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})


