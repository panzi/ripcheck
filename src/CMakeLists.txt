if(WITH_VISUALIZE)
	add_definitions(-DWITH_VISUALIZE)
	link_directories(${LIBPNG_LIBRARY_DIRS})
	include_directories(${LIBPNG_INCLUDE_DIRS})
	set(visulaize_SRCS print_image.c)
endif()

add_executable(ripcheck 
	main.c
	print_text.c
	ripcheck.c
	${visulaize_SRCS})

if(WITH_VISUALIZE)
	target_link_libraries(ripcheck ${LIBPNG_LIBRARIES})
endif()

set_target_properties(ripcheck PROPERTIES COMPILE_FLAGS ${CMAKE_CXX_FLAGS})

install(TARGETS ripcheck
	RUNTIME DESTINATION bin)
