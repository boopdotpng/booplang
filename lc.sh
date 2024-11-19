#!/bin/sh

# directories to search
SRC_DIRS="src include examples"

# file extensions to include
EXTENSIONS="c h"

# find and count lines
TOTAL=$(find $SRC_DIRS -type f \( -name "*.c" -o -name "*.h" \) -exec wc -l {} + | awk '{sum += $1} END {print sum}')
echo "total line count: $TOTAL"
