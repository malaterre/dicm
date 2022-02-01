# tests
set(TEST_SRCS
  testdicm_de.c
  )

create_test_sourcelist(dicmtest dicmtest.c ${TEST_SRCS})
add_executable(dicmtest ${dicmtest})
#target_include_directories(dicmtest ${dicm_SOURCE_DIR}/src)
include_directories(${dicm_SOURCE_DIR}/src)
target_link_libraries(dicmtest dicm)

foreach(name ${TEST_SRCS})
  get_filename_component(testname ${name} NAME_WE)
  add_test(NAME ${testname} COMMAND dicmtest ${testname})
endforeach()


