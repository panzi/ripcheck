# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

set(COMPILER_PREFIX "i686-w64-mingw32" CACHE STRING "Prefix of compiler binaries")

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH /usr/${COMPILER_PREFIX})

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   ${COMPILER_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${COMPILER_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${COMPILER_PREFIX}-windres)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
