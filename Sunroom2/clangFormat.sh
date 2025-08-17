#!/bin/bash

# Usage:
#   ./clangFormat.sh [--all] [file]
# If --all is given, format all C++ files in src/
# If a file is given, format that file
# If neither is given, print usage

if [[ "$1" == "--all" ]]; then
    # Format all C++ files in the project
    find src/ \( -name "*.cpp" -o -name "*.h" \) | xargs clang-format -i
elif [[ -n "$1" ]]; then
    # Format the specified file
    clang-format -i "$1"
else
    echo "Usage: $0 [--all] [file]"
    echo "  --all    Format all C++ files in src/"
    echo "  file     Format the specified file"
    exit 1
fi