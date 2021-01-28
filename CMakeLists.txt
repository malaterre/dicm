cmake_minimum_required(VERSION 3.14)
project(dicm 
  VERSION 0.0.1
  LANGUAGES C
)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
#message(${dicm_VERSION_MAJOR})
#message(${dicm_VERSION_MINOR})
#message(${dicm_VERSION_PATCH})

add_subdirectory(src)
add_subdirectory(examples)

# need to be at top-level
enable_testing()

add_subdirectory(tests)
