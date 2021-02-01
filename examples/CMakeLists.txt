include_directories(${dicm_SOURCE_DIR}/src)
add_library(dicm-default STATIC
mem-ansi.c
#log4c.c
dlog.c
file.c
copy.c
)
add_executable(example1 example1.c
default.c
event.c
dcmdump.c
)
target_link_libraries(example1 dicm dicm-default)

set(DICOM_FILES
#exp-defsq.dcm
#exp-emptydefsq.dcm
#exp-emptysq.dcm
#exp-emptysq-group.dcm
#exp-icon-frag.dcm
#exp-icon-frag-ul.dcm
#exp-jls.dcm
#exp-nestedsq.dcm
#exp-nestedsq-jls2.dcm
#exp-nestedsq-jls.dcm
#exp-nestedsq-jls-ul.dcm
#exp-sq.dcm
#fix.dcm
#garbage.dcm
#imp.dcm
#input.dcm
input-group.dcm
#output.dcm
)

foreach(DICOM_FILE ${DICOM_FILES})
  add_test(NAME example1_${DICOM_FILE} COMMAND example1 "" ${DICM_TEST_DATA_ROOT}/${DICOM_FILE})
endforeach()
