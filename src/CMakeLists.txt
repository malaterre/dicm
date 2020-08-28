set(dicm_SRCS
 dicm.c
 dicm-parser.c
 dicm-log.c
)

add_library(dicm
 SHARED
 ${dicm_SRCS}
)
