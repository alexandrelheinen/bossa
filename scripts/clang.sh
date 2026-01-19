#!/bin/bash

# Source the env.sh for the project name
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "${CURDIR}/env.sh"

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "clang-format could not be found. Please install it to format the code."
    exit 1
fi

# Format all .cpp and .h files in src/ and include/ directories
echo -e "Formatting the \e[34m${PROJECT_NAME}\e[0m files..."
for file in $(find "${PROJECT_DIR}/include" "${PROJECT_DIR}/src" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \)); do
    echo "-- ${file}"
    clang-format -i "$file"
done