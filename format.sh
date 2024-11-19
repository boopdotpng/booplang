#!/bin/bash

dirs=("include" "src")

for dir in "${dirs[@]}"; do
    find "$dir" -type f \( -name "*.c" -o -name "*.h" \) -exec astyle --style=allman --quiet {} \;
done

find "${dirs[@]}" -type f -name "*.orig" -exec rm -f {} \; &>/dev/null
