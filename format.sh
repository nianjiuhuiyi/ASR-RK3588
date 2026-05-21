#!/usr/bin/env bash

# 指定文件夹路径
DIR="./src"

find $DIR -name '*.cpp' -o -name '*.h' -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" | while read -r file
do
    clang-format -i "$file"
done
