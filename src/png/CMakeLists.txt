project(libpng C)

file(GLOB LIBPNG_SRC "*.c")

# add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} STATIC ${LIBPNG_SRC})
target_compile_definitions(${PROJECT_NAME} PUBLIC SPNG_STATIC)
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

