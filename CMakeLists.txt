cmake_minimum_required(VERSION 2.8.11)

project(VKraft)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}/glfw/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/VKL/include"
)

option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glfw")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/VKL")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")
