cmake_minimum_required(VERSION 3.10)

project(asr_rknn_serve)

# install target and libraries 
set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/utils/ utils.out)

set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/zipformer)


file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/zipformer/*.cpp)

add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME}
    fileutils
    audioutils
    ${LIBRKNNRT}
    ${LIBKALDI_NATIVE_FBANK}
    http
    spdlog::spdlog
)


if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBRKNNRT_INCLUDES}
    ${LIBKALDI_NATIVE_FBANK_INCLUDES}
    ${LIBTIMER_INCLUDES}
    ${LIBALSA_INCLUDES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(PROGRAMS ${LIBRKNNRT} DESTINATION lib)

# resources
# file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../model/*.rknn")
# install(FILES ${RKNN_FILES} DESTINATION model)
install(DIRECTORY resources DESTINATION .)

install(PROGRAMS run.sh DESTINATION .)
install(PROGRAMS stop.sh DESTINATION .)
