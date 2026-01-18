#!/bin/bash

# Define a function for the usage message
SCRIPT_NAME=$(basename $0)
usage() {
    echo "Usage: ${SCRIPT_NAME} [-t TARGET] [-d REMOTE_DIR] [-a ARTIFACT_PATH]"
    echo "  TARGET: Optional. The target device to build for. Default is ${DEFAULT_TARGET}"
    echo "  REMOTE_DIR: Optional. The remote directory to copy the build artifacts to. Default is ${DEFAULT_REMOTE_DIR}"
    echo "  ARTIFACT_PATH: Optional. The path to the build artifact to sync. Default is the latest build artifact in the build/ directory."
    echo ""
    exit 0
}

# Parse the arguments
while getopts "t:d:h:" opt; do
    case ${opt} in
        t )
            TARGET=${OPTARG}
            ;;
        d )
            REMOTE_DIR=${OPTARG}
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

# Set defaults if not provided
DEFAULT_TARGET="pi@raspberry.local"
DEFAULT_REMOTE_DIR="/opt/${PROJECT_NAME}"
DEFAULT_ARTIFACT_PATH="${PROJECT_DIR}/build/${PROJECT_NAME}.tar.gz"

TARGET="${TARGET:-$DEFAULT_TARGET}"
REMOTE_DIR="${REMOTE_DIR:-$DEFAULT_REMOTE_DIR}"
ARTIFACT_PATH="${ARTIFACT_PATH:-$DEFAULT_ARTIFACT_PATH}"

echo -e "Syncing the \e[34m${PROJECT_NAME}\e[0m project to the target device..."
echo "-- TARGET: ${TARGET}"
echo "-- REMOTE_DIR: ${REMOTE_DIR}"
echo "-- ARTIFACT_PATH: ${ARTIFACT_PATH}"

# Copy the artifact to the target device
echo "Copying artifact to the target device..."
scp "${ARTIFACT_PATH}" "${TARGET}:/tmp/${PROJECT_NAME}.tar.gz"
if [ $? -ne 0 ]; then
    echo -e "\e[31mFailed to copy the artifact to the target device.\e[0m"
    exit 1
fi

# SSH into the target device and deploy the artifact
echo "Deploying artifact on the target device..."
ssh "${TARGET}" << EOF
    sudo mkdir -p "${REMOTE_DIR}"
    sudo tar xzf "/tmp/${PROJECT_NAME}.tar.gz" -C "${REMOTE_DIR}"
    sudo chmod +x "${REMOTE_DIR}/bin/${PROJECT_NAME}"
    sudo rm "/tmp/${PROJECT_NAME}.tar.gz"
EOF

# Copy the systemd service file and enable it
echo "Setting up the systemd service on the target device..."
SERVICE_FILE="${PROJECT_DIR}/config/${PROJECT_NAME}.service"

REMOTE_SERVICE_PATH="/etc/systemd/system/${PROJECT_NAME}.service"
scp "${SERVICE_FILE}" "${TARGET}:/tmp/${PROJECT_NAME}.service"
ssh "${TARGET}" << EOF
    sudo mv "/tmp/${PROJECT_NAME}.service" "${REMOTE_SERVICE_PATH}"
    sudo chown root:root "${REMOTE_SERVICE_PATH}"
    sudo chmod 644 "${REMOTE_SERVICE_PATH}"
    sudo systemctl daemon-reload
    sudo systemctl enable "${PROJECT_NAME}.service"
    sudo systemctl restart "${PROJECT_NAME}.service"
EOF

echo -e "\e[34m${PROJECT_NAME}\e[0m project synced and deployed successfully to ${TARGET}."