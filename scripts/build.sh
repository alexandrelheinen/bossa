#!/bin/bash

# Define a function for the usage message
SCRIPT_NAME=$(basename $0)
usage() {
    echo "Usage: ${SCRIPT_NAME} [-t TOOLCHAIN]"
    echo "  TOOLCHAIN: Optional. Path to the toolchain file to use for cross-compilation."
    echo ""
    exit 0
}

# Parse the arguments
while getopts "t:h" opt; do
    case ${opt} in
        t )
            TOOLCHAIN=${OPTARG}
            ;;
        h )
            usage
            ;;
        \? )
            usage
            ;;
    esac
done

# Source the env.sh for the project name
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "${CURDIR}/env.sh"

# Check for cross-compilation argument
if [ -z "${TOOLCHAIN}" ]; then
    echo -e "Building the \e[34m${PROJECT_NAME}\e[0m project (native build)"
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
else
    if [ ! -f "${PROJECT_DIR}/${TOOLCHAIN}" ]; then
        echo -e "\e[31mError: Toolchain file not found: ${TOOLCHAIN}\e[0m"
        exit 1
    fi
    echo -e "Building the \e[34m${PROJECT_NAME}\e[0m project (cross-compile with: ${TOOLCHAIN})"
    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../${TOOLCHAIN}"
fi

BUILD_DIR="${PROJECT_DIR}/build"
mkdir -p ${BUILD_DIR} && cd ${BUILD_DIR}
cmake ${CMAKE_ARGS} ..
make -j$(nproc)
chmod +x "${BUILD_DIR}/final/bin/${PROJECT_NAME}"
ARTIFACT_PATH="${BUILD_DIR}/${PROJECT_NAME}.tar.gz"
tar czf ${ARTIFACT_PATH} -C ${BUILD_DIR}/final bin
echo "Build completed"
echo "-- Artifact: ${ARTIFACT_PATH}"