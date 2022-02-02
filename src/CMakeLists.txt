include(GenerateExportHeader)

set(dicm_SRCS dicm-de.c dicm-log.c dicm-parser.c dicm-parser-exp.c dicm-uid.c
              dicm.c)

add_library(dicm SHARED ${dicm_SRCS})
generate_export_header(dicm)
target_include_directories(dicm PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# https://interrupt.memfault.com/blog/best-and-worst-gcc-clang-compiler-flags
set(flags
    -Wall
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

target_compile_options(
  dicm
  PRIVATE # clang/gcc warnings:
          "$<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:GNU>>:${flags}>"
          # MSVC warnings:
          "$<$<C_COMPILER_ID:MSVC>:/W4>")
