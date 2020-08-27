set(dicm_SRCS
 dicm.c
 parser.c
)

add_library(dicm
 SHARED
 ${dicm_SRCS}
)
