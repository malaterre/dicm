set(dicm_SRCS
 dicm-de.c
 dicm-log.c
 dicm-parser.c
 dicm-parser-exp.c
 dicm-uid.c
 dicm.c
)

add_library(dicm
 SHARED
 ${dicm_SRCS}
)
