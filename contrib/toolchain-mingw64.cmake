# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_COMPILER_IS_MINGW64 TRUE)

set(COMPILER_PREFIX "x86_64-w64-mingw32" CACHE STRING "Prefix of compiler binaries")

# change this to your mingw64 installation path
set(MINGW64_PATH "/usr/${COMPILER_PREFIX}" CACHE PATH "Path to your mingw64 installation")

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH ${MINGW64_PATH} ${MINGW64_PATH}/mingw)

# which compilers to use for C and C++
find_program(CMAKE_C_COMPILER   NAMES ${COMPILER_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER NAMES ${COMPILER_PREFIX}-g++)
find_program(CMAKE_RC_COMPILER  NAMES ${COMPILER_PREFIX}-windres)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
