#!/bin/bash

# Source the env.sh to set up environment variables
CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "${CURDIR}/env.sh"

# Print the project name in blue color
echo -e "Setting \e[34m${PROJECT_NAME}\e[0m project requirements..."
echo ""

REQUIREMENTS="build-essential \
                cmake \
                g++ \
                make \
                tar \
                rsync \
                openssh-server \
                gcc-aarch64-linux-gnu \
                g++-aarch64-linux-gnu"

sudo apt update
sudo apt install -y ${REQUIREMENTS}
echo ""

# Verify installation
echo "Verifying required commands..."
REQUIRED_COMMANDS=("cmake" "g++" "make" "tar" "rsync" "ssh" "aarch64-linux-gnu-gcc" "aarch64-linux-gnu-g++")
is_installed=true
for cmd in "${REQUIRED_COMMANDS[@]}"; do
    if ! command -v $cmd &> /dev/null; then
        echo -e "  \e[31m$cmd\e[0m is not installed."
        is_installed=false
    else
        echo -e "  \e[34m$cmd\e[0m is installed."
    fi
done

if [ "$is_installed" = false ]; then
    echo ""
    echo -e "\e[31mSome required commands are missing.\e[0m"
    echo "Please install them and re-run the setup."
    exit 1
else
    echo "Installation verified successfully."
fi