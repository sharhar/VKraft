CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

file(GLOB_RECURSE GRAPHER_SRC *.cpp)
file(GLOB_RECURSE GRAPHER_H *.h)

include_directories(
	"${CMAKE_SOURCE_DIR}/GLUI/ext/glcorearb"
	"${CMAKE_SOURCE_DIR}/src"
)

if(MSVC)
	add_executable(VKraft ${GRAPHER_H} ${GRAPHER_SRC})
	add_compile_definitions(UTIL_DIR_PRE="../../")
else()
	add_executable(VKraft ${GRAPHER_H} ${GRAPHER_SRC})
endif()

target_link_libraries (VKraft LINK_PUBLIC VKL)
target_link_libraries (VKraft glfw ${GLFW_LIBRARIES})
