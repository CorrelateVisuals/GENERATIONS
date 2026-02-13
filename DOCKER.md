# Docker Support for GENERATIONS

This document describes how to run GENERATIONS in Docker with GPU/Vulkan support on various platforms.

## Prerequisites

- Docker Engine 20.10+ or Docker Desktop
- For NVIDIA GPUs: [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html)
- For display forwarding: X11 server access

## Quick Start

### Using the Helper Script (Recommended)

The easiest way to run GENERATIONS in Docker:

```bash
# For AMD/Intel/Mesa GPU
./run-docker.sh mesa

# For NVIDIA GPU
./run-docker.sh nvidia

# For Raspberry Pi
./run-docker.sh raspberry-pi
```

### Manual Docker Compose

Alternatively, use docker-compose directly:

#### For Systems with AMD/Intel/Mesa GPU

```bash
# Allow X11 connections from Docker
xhost +local:docker

# Build and run
docker-compose --profile mesa up --build
```

#### For Systems with NVIDIA GPU

```bash
# Allow X11 connections from Docker
xhost +local:docker

# Build and run
docker-compose --profile nvidia up --build
```

#### For Raspberry Pi (ARM64)

```bash
# Allow X11 connections from Docker
xhost +local:docker

# Build and run
docker-compose --profile raspberry-pi up --build
```

## Detailed Instructions

### Building the Docker Image

#### Standard x86_64 Build
```bash
docker build -t generations:latest .
```

#### ARM64/Raspberry Pi Build
```bash
docker build -f Dockerfile.arm64 -t generations:arm64 .
```

### Running the Container

#### Basic Run (Mesa/AMD/Intel)
```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --device /dev/dri:/dev/dri \
  generations:latest
```

#### NVIDIA GPU
```bash
docker run --rm -it \
  --runtime=nvidia \
  -e DISPLAY=$DISPLAY \
  -e NVIDIA_VISIBLE_DEVICES=all \
  -e NVIDIA_DRIVER_CAPABILITIES=all \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --device /dev/dri:/dev/dri \
  generations:latest
```

#### Raspberry Pi
```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --device /dev/dri:/dev/dri \
  --device /dev/vchiq:/dev/vchiq \
  generations:arm64
```

## Platform-Specific Setup

### Linux

1. **Install Docker:**
   ```bash
   curl -fsSL https://get.docker.com -o get-docker.sh
   sudo sh get-docker.sh
   sudo usermod -aG docker $USER
   ```

2. **For NVIDIA GPUs:**
   ```bash
   # Install NVIDIA Container Toolkit
   distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
   curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
   curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | \
     sudo tee /etc/apt/sources.list.d/nvidia-docker.list
   
   sudo apt-get update
   sudo apt-get install -y nvidia-container-toolkit
   sudo systemctl restart docker
   ```

3. **Allow X11 forwarding:**
   ```bash
   xhost +local:docker
   ```

### Raspberry Pi

1. **Install Docker on Raspberry Pi OS:**
   ```bash
   curl -fsSL https://get.docker.com -o get-docker.sh
   sudo sh get-docker.sh
   sudo usermod -aG docker $USER
   ```

2. **Enable Vulkan support:**
   ```bash
   # Update system
   sudo apt-get update && sudo apt-get upgrade -y
   
   # Install Mesa Vulkan drivers
   sudo apt-get install -y mesa-vulkan-drivers vulkan-tools
   
   # Test Vulkan
   vulkaninfo
   ```

3. **Allow X11 forwarding:**
   ```bash
   xhost +local:docker
   ```

4. **Build and run:**
   ```bash
   docker-compose --profile raspberry-pi up --build
   ```

### Windows (with WSL2)

1. **Install WSL2 and Docker Desktop**
2. **Enable WSLg for GUI support** (included in Windows 11)
3. **Run with Docker Desktop:**
   ```bash
   # In WSL2 terminal
   docker-compose --profile mesa up --build
   ```

## Troubleshooting

### Display Not Working

If the application fails to connect to the display:

```bash
# Allow all X11 connections (less secure, for testing only)
xhost +

# Or allow specific container
xhost +local:docker
```

### GPU Not Detected

1. **Verify Vulkan is working on host:**
   ```bash
   vulkaninfo
   ```

2. **Check device permissions:**
   ```bash
   ls -la /dev/dri
   # Should show video group access
   ```

3. **For NVIDIA, verify runtime:**
   ```bash
   docker run --rm --runtime=nvidia nvidia/cuda:11.8.0-base-ubuntu22.04 nvidia-smi
   ```

### Permission Denied on /dev/dri

```bash
# Add user to video group
sudo usermod -aG video $USER
# Log out and back in for changes to take effect
```

### Raspberry Pi Specific Issues

1. **VideoCore device not found:**
   ```bash
   # Check if device exists
   ls -la /dev/vchiq
   
   # Add user to video group
   sudo usermod -aG video $USER
   ```

2. **Out of memory errors:**
   ```bash
   # Increase GPU memory in /boot/config.txt
   sudo nano /boot/config.txt
   # Add: gpu_mem=256
   sudo reboot
   ```

## Performance Optimization

### For Production Use

Build with optimizations:
```bash
docker build \
  --build-arg CMAKE_BUILD_TYPE=Release \
  -t generations:optimized .
```

### Limiting Resources

```bash
docker run --rm -it \
  --cpus="2" \
  --memory="2g" \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --device /dev/dri:/dev/dri \
  generations:latest
```

## Docker Compose Profiles

The `docker-compose.yml` includes several profiles:

- `mesa` - For AMD/Intel/Mesa GPUs (default)
- `nvidia` - For NVIDIA GPUs with nvidia-container-runtime
- `arm64` / `raspberry-pi` - For ARM64 devices like Raspberry Pi

Activate a profile:
```bash
docker-compose --profile <profile-name> up
```

## Verifying Vulkan Support

Inside the container:
```bash
docker run --rm -it \
  --device /dev/dri:/dev/dri \
  generations:latest \
  bash -c "apt-get update && apt-get install -y vulkan-tools && vulkaninfo"
```

## Security Notes

- `xhost +local:docker` allows Docker containers to access your X server
- For production, use more restrictive X11 authentication
- Consider using VNC or other remote display methods for better security
- The containers run as root by default; consider adding a non-root user for production

## Additional Resources

- [Vulkan on Linux](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started.html)
- [Docker GPU Support](https://docs.docker.com/config/containers/resource_constraints/#gpu)
- [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/)
- [Raspberry Pi Vulkan](https://www.raspberrypi.com/documentation/computers/vulkan.html)
