set(dicm_SRCS dicm-de.c dicm-log.c dicm-parser.c dicm-parser-exp.c dicm-uid.c
              dicm.c)

add_library(dicm SHARED ${dicm_SRCS})

# https://interrupt.memfault.com/blog/best-and-worst-gcc-clang-compiler-flags
target_compile_options(
  dicm
  PRIVATE -Wall
          -Wextra
          # -Werror
          -Wshadow
          -Wdouble-promotion
          -Wformat=2
          -Wformat-truncation
          -Wundef
          -fno-common
          # -fstack-usage
          -Wstack-usage=512
          -Wconversion
          -Wunused-parameter)
