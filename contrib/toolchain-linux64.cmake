# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)

# Which compilers to use for C and C++
set(CMAKE_C_COMPILER gcc -m64)
set(CMAKE_CXX_COMPILER g++ -m64)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
