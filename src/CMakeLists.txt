CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

file(GLOB_RECURSE VKRAFT_SRC *.cpp)
file(GLOB_RECURSE VKRAFT_H *.h)

include_directories(
	"${CMAKE_SOURCE_DIR}/src"
)

add_executable(VKraft ${VKRAFT_H} ${VKRAFT_SRC})

if(MSVC)
	add_compile_definitions(UTIL_DIR_PRE="../../")
	add_compile_definitions(VKL_SURFACE_WIN32)
elseif(APPLE)
	add_compile_definitions(UTIL_DIR_PRE="../../../")
	add_compile_definitions(VKL_SURFACE_MACOS)
	
	find_path(QUARTZCORE_INCLUDE_DIR QuartzCore/QuartzCore.h)
	find_library(QUARTZCORE_LIBRARY QuartzCore)

	target_link_libraries (VKraft ${QUARTZCORE_LIBRARY})
endif()

target_link_libraries (VKraft LINK_PUBLIC VKL)
target_link_libraries (VKraft glfw ${GLFW_LIBRARIES})
