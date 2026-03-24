cmake_minimum_required(VERSION 3.8)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(spreadsheet CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(antlr4-runtime REQUIRED)

include_directories(
    ${ANTLR4_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Formula
)

file(GLOB sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Formula/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Formula/*.h
    src/*.cpp
    src/*.h
)

add_executable(
    spreadsheet
    ${sources}
)

target_link_libraries(spreadsheet antlr4_static)