CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

file(GLOB_RECURSE GRAPHER_SRC *.cpp)
file(GLOB_RECURSE GRAPHER_H *.h)

include_directories(
	"${CMAKE_SOURCE_DIR}/GLUI/ext/glcorearb"
	"${CMAKE_SOURCE_DIR}/src"
)

if(MSVC)
	add_executable(VKraft ${GRAPHER_H} ${GRAPHER_SRC})
	file(COPY "${CMAKE_SOURCE_DIR}/res" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
	file(COPY "${CMAKE_SOURCE_DIR}/res" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release")
	file(COPY "${CMAKE_SOURCE_DIR}/res" DESTINATION "${CMAKE_BINARY_DIR}/src")
else()
	add_executable(VKraft ${GRAPHER_H} ${GRAPHER_SRC})
	file(COPY "${CMAKE_SOURCE_DIR}/src/arial.ttf" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

target_link_libraries (VKraft LINK_PUBLIC VKL)
target_link_libraries (VKraft glfw ${GLFW_LIBRARIES})
