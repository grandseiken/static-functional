#!/bin/bash
OIFS="$IFS"
IFS=$'\n'
for file in $(git ls-files | grep "\.cc$\|\.h$"); do
  clang-format -i $file
done
