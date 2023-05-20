project(libsvg)

file(GLOB LIBSVG_SRC "*.c" "*.cpp")

# add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${LIBSVG_SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

