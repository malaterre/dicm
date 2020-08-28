include_directories(${dicm_SOURCE_DIR}/src)
add_executable(example1 example1.c
mem-ansi.c
#log4c.c
dlog.c
)
target_link_libraries(example1 dicm)
