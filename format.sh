#!/bin/sh

list=$(git ls-files -- src tests examples | grep -v CMakeLists.txt)
echo $list

for f in $list; do
clang-tidy -checks='-*,google-readability-casting,readability-string-compare,readability-simplify-boolean-expr,readability-braces-around-statements,modernize-use-nullptr' -fix-errors $f -- -std=c11
clang-format -style=file -i $f
done

list=$(git ls-files -- . | grep CMakeLists.txt)
echo $list

for f in $list
do cmake-format -i $f
done

# clang-tidy --list-checks -checks='*' | grep "modernize"
