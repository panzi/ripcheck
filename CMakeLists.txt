cmake_minimum_required(VERSION 2.0)

project(ripcheck)

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(LIBPNG libpng)
	if(LIBPNG_FOUND)
		option(WITH_VISUALIZE "Build with visualization support" ON)
	else()
		option(WITH_VISUALIZE "Build with visualization support" OFF)
	endif()
else()
	option(WITH_VISUALIZE "Build with visualization support" OFF)
endif()

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror -std=gnu99 -O2")
endif()

add_subdirectory(src)

# uninstall target
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
