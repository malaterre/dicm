cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
project(dicm)

add_executable(tvr testvr.cxx vr.cxx)
# how to I require cstdint ? instead require another one we need:
target_compile_features(tvr PRIVATE cxx_strong_enums)
