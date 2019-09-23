cmake_minimum_required(VERSION 2.8)
project (SWRasterizer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} O3 -w")

file(GLOB PROJECT_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.cpp")

include_directories(SWRasterizer ${PROJECT_SOURCE_DIR}/include)
include_directories(SWRasterizer ${PROJECT_SOURCE_DIR}/include/SDL2)

set(SDL2_LIBRARIES_NAMES 
    "SDL2main"
    "SDL2")

set (LIB_DIR ${PROJECT_SOURCE_DIR}/lib/win32/x86)
set (LIBS ${LIBS} ${LIB_DIR}/SDL2/SDL2.lib)
set (LIBS ${LIBS} ${LIB_DIR}/SDL2/SDL2main.lib)
set (LIBS ${LIBS} ${LIB_DIR}/SDL2_ttf.lib)

add_executable (${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} ${LIBS})