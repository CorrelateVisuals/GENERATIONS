#!/bin/bash
# Helper script to run GENERATIONS in Docker with GPU support
# Usage: ./run-docker.sh [nvidia|mesa|raspberry-pi|dev]
#
# If you get a "permission denied" error, make this script executable:
#   chmod +x run-docker.sh

set -e

GPU_TYPE="${1:-mesa}"

echo "Running GENERATIONS with $GPU_TYPE GPU support..."

# Allow X11 connections from Docker
xhost +local:docker > /dev/null 2>&1 || echo "Note: Could not run xhost. X11 forwarding may not work."

case "$GPU_TYPE" in
  nvidia)
    echo "Starting with NVIDIA GPU support..."
    docker-compose --profile nvidia up
    ;;
  mesa|amd|intel)
    echo "Starting with Mesa/AMD/Intel GPU support..."
    docker-compose --profile mesa up
    ;;
  raspberry-pi|arm64|rpi)
    echo "Starting with Raspberry Pi/ARM64 support..."
    docker-compose --profile raspberry-pi up
    ;;
  dev|development)
    echo "Starting development container with build tools..."
    echo "Repository mounted at /workspace - you can build and run inside the container"
    docker-compose --profile dev up
    ;;
  *)
    echo "Unknown GPU type: $GPU_TYPE"
    echo "Usage: $0 [nvidia|mesa|raspberry-pi|dev]"
    echo ""
    echo "Options:"
    echo "  nvidia       - NVIDIA GPU with nvidia-container-runtime"
    echo "  mesa         - AMD/Intel GPU with Mesa drivers"
    echo "  raspberry-pi - Raspberry Pi with VideoCore GPU"
    echo "  dev          - Development container with git and build tools"
    exit 1
    ;;
esac
