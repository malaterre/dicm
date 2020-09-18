include_directories(${dicm_SOURCE_DIR}/src)
add_library(dicm-default STATIC
mem-ansi.c
#log4c.c
dlog.c
file.c
)
add_executable(example1 example1.c
default.c
event.c
dcmdump.c
)
target_link_libraries(example1 dicm dicm-default)
