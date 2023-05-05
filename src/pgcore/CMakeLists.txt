project(pgcore)

file(GLOB PGCORE_SRC "*.c")

add_library(${PROJECT_NAME} ${PGCORE_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

