cmake_minimum_required(VERSION 3.14)
project(
  dicm
  VERSION 0.0.1
  LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(src)

find_path(
  DICM_TEST_DATA_ROOT "input-group.dcm"
  PATHS $ENV{DICM_TEST_DATA_ROOT} ${PROJECT_SOURCE_DIR}/../dicm-test-data
  NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
mark_as_advanced(DICM_TEST_DATA_ROOT)

# need to be at top-level
enable_testing()

add_subdirectory(examples)
add_subdirectory(tests)
