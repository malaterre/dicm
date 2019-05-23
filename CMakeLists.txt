cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)
if (POLICY CMP0063)
  cmake_policy (SET CMP0063 NEW)
endif (POLICY CMP0063)
project(dicm)

set(LIBRARY_INSTALL_DIR lib)
set(INCLUDE_INSTALL_DIR include)

include(GenerateExportHeader)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
add_library(dicm SHARED vr.cxx)
#set_target_properties(dicm PROPERTIES CXX_VISIBILITY_PRESET hidden)
generate_export_header(dicm)
install(TARGETS dicm DESTINATION ${LIBRARY_INSTALL_DIR})
install(FILES
 value_representation.hxx
 ${dicm_BINARY_DIR}/dicm_export.h
 DESTINATION ${INCLUDE_INSTALL_DIR}
)
# how to I require cstdint ? instead require another one we need:
target_compile_features(dicm PUBLIC cxx_strong_enums)
target_include_directories(dicm PUBLIC ${dicm_BINARY_DIR})

#find_package(GTest REQUIRED)
#message(${GTEST_INCLUDE_DIRS})
add_subdirectory (/usr/src/gtest gtest-build)

add_executable(tvr testvr.cxx)
target_link_libraries(tvr dicm gtest gtest_main)
enable_testing()
add_test(tvr tvr)
