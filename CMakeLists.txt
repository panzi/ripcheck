cmake_minimum_required(VERSION 2.0)

project(ripcheck)

include(CheckFunctionExists)

check_function_exists(strlcpy HAVE_STRLCPY)

find_package(PkgConfig)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/libpng/" AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/contrib/zlib/")
		option(WITH_VISUALIZE "Build with visualization support" ON)
	else()
		option(WITH_VISUALIZE "Build with visualization support" OFF)
		message("
To build visualization support for Windows binaries please download
libpng (libpng-1.2.*-lib.zip) and zlib (zlib1*-dll.zip) and extract
these archives to contrib/libpng and contrib/zlib.

In order to run the resulting binary you need libpng12.dll (download
libpng-1.2.*-bin.zip) and zlib1.dll (included in zlib1*-dll.zip).

See:
http://www.libpng.org/pub/png/libpng.html
http://www.zlib.net/
")
	endif()
	if(WITH_VISUALIZE)
		set(LIBPNG_INCLUDE_DIRS
			"${CMAKE_SOURCE_DIR}/contrib/libpng/include;${CMAKE_SOURCE_DIR}/contrib/zlib/include")
		set(LIBPNG_LIBRARY_DIRS
			"${CMAKE_SOURCE_DIR}/contrib/libpng/lib;${CMAKE_SOURCE_DIR}/contrib/zlib/lib")
		set(LIBPNG_LIBRARIES png12)
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
