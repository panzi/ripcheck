cmake_minimum_required(VERSION 2.0)

project(ripcheck C)

include(CheckFunctionExists)

check_function_exists(strlcpy HAVE_STRLCPY)

find_package(PkgConfig)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/libpng/" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/zlib/")
		option(WITH_VISUALIZE "Build with visualization support" ON)
	else()
		option(WITH_VISUALIZE "Build with visualization support" OFF)
	endif()

	set(CMAKE_EXE_LINKER_FLAGS "-static")

	if(WITH_VISUALIZE)
		set(LIBPNG_LIBRARIES png16 z)
	endif()
elseif(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPNG libpng)
	if(LIBPNG_FOUND)
		option(WITH_VISUALIZE "Build with visualization support" ON)
	else()
		option(WITH_VISUALIZE "Build with visualization support" OFF)
	endif()
else()
	option(WITH_VISUALIZE "Build with visualization support" OFF)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -std=gnu99")

	if(NOT(CMAKE_BUILD_TYPE STREQUAL "Debug"))
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
	endif()

	if(NOT(CMAKE_COMPILER_IS_MINGW64))
		# can't be pedantic for mingw64 because of format strings
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
	endif()
endif()

add_subdirectory(src)

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
