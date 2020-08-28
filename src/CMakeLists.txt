set(dicm_SRCS
 dicm.c
 parser.c
 dicm-log.c
)

add_library(dicm
 SHARED
 ${dicm_SRCS}
)
