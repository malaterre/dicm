include_directories(${dicm_SOURCE_DIR}/src)
add_library(
  dicm-default STATIC
  # log4c.c
  dlog.c file.c copy.c)
target_compile_definitions(
  dicm-default PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
target_link_libraries(dicm-default dicm)
add_executable(example1 example1.c default.c event.c json.c dcmdump.c)
target_link_libraries(example1 dicm-default)

add_executable(dicm2json dicm2json.c default.c json.c json.c dcmdump.c)
target_link_libraries(dicm2json dicm dicm-default)

set(DICOM_FILES
    exp-defsq.dcm
    exp-emptydefsq.dcm
    exp-emptysq.dcm
    exp-emptysq-group.dcm
    exp-icon-frag.dcm
    exp-icon-frag-ul.dcm
    exp-jls.dcm
    exp-nestedsq.dcm
    exp-nestedsq-jls2.dcm
    exp-nestedsq-jls.dcm
    exp-nestedsq-jls-ul.dcm
    exp-sq.dcm
    # fix.dcm garbage.dcm imp.dcm
    input.dcm
    input-group.dcm
    # output.dcm
)

add_test(NAME example1_group_length1
         COMMAND example1 "" ${DICM_TEST_DATA_ROOT}/input-group.dcm)
add_test(NAME example1_group_length2
         COMMAND example1 "gl" ${DICM_TEST_DATA_ROOT}/input-group.dcm)
add_test(NAME example1_group_length3
         COMMAND example1 "fme" ${DICM_TEST_DATA_ROOT}/input-group.dcm)
add_test(NAME example1_group_length4
         COMMAND example1 "all" ${DICM_TEST_DATA_ROOT}/input-group.dcm)

foreach(DICOM_FILE ${DICOM_FILES})
  add_test(NAME example1_${DICOM_FILE}
           COMMAND example1 "" ${DICM_TEST_DATA_ROOT}/${DICOM_FILE})
endforeach()
