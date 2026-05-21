cmake_minimum_required(VERSION 3.4...3.14)

# 特别是涉及到交叉编译的话，这下面一定要指定 LANGUAGES CXX
# 这可以是 header-only进行使用的
project(http LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
