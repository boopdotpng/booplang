#!/bin/bash

dirs=("include" "src")

for dir in "${dirs[@]}"; do
    find "$dir" -type f \( -name "*.c" -o -name "*.h" \) -exec astyle --style=allman {} \;
done

find "${dirs[@]}" -type f -name "*.orig" -exec rm -f {} \;

echo "formatted files in include/ and src/, removed .orig backups!"
