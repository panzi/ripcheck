if(WITH_VISUALIZE)
	add_definitions(-DWITH_VISUALIZE)
	link_directories(${LIBPNG_LIBRARY_DIRS})
	include_directories(${LIBPNG_INCLUDE_DIRS})
	set(visulaize_SRCS print_image.c print_image.h)
endif()

if(HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	set(strlcpy_SRCS strlcpy.c)
endif()

add_executable(ripcheck
	main.c
	print_text.c
	ripcheck.c
	print_text.h
	ripcheck.h
	ripcheck_endian.h
	${visulaize_SRCS}
	${strlcpy_SRCS})

if(WITH_VISUALIZE)
	target_link_libraries(ripcheck ${LIBPNG_LIBRARIES})
endif()

install(TARGETS ripcheck
	RUNTIME DESTINATION bin)
