cmake_minimum_required(VERSION 3.14)
project(
  dicm
  VERSION 0.0.1
  LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

# good default for executables:
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
# only expose shared option to advanced users:
mark_as_advanced(BUILD_SHARED_LIBS)
# Define visibility settings globally (for now?)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

include(GNUInstallDirs)

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
