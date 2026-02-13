# Running GENERATIONS on Raspberry Pi with Docker

This guide specifically covers running GENERATIONS on Raspberry Pi devices with Vulkan GPU support.

## Supported Hardware

- **Raspberry Pi 4** (2GB RAM minimum, 4GB+ recommended)
- **Raspberry Pi 5** (recommended for better performance)
- **Raspberry Pi 400** (similar to Pi 4)

## Prerequisites

### 1. Update Your Raspberry Pi

```bash
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get dist-upgrade -y
sudo reboot
```

### 2. Install Docker

```bash
# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add your user to the docker group
sudo usermod -aG docker $USER

# Log out and back in, or run:
newgrp docker
```

### 3. Enable Vulkan Support

Raspberry Pi 4 and 5 have VideoCore GPUs that support Vulkan through Mesa drivers.

```bash
# Install Mesa Vulkan drivers
sudo apt-get install -y mesa-vulkan-drivers vulkan-tools

# Verify Vulkan is working
vulkaninfo | grep "deviceName"
```

You should see output mentioning "V3D" or "VideoCore" if Vulkan is working correctly.

### 4. Increase GPU Memory (Optional but Recommended)

```bash
# Edit boot configuration
sudo nano /boot/config.txt

# Add or modify this line (256MB is recommended)
gpu_mem=256

# Save and reboot
sudo reboot
```

## Running GENERATIONS

### Method 1: Using the Helper Script (Easiest)

```bash
# Clone the repository
git clone https://github.com/CorrelateVisuals/GENERATIONS.git
cd GENERATIONS

# Allow X11 connections
xhost +local:docker

# Run with the helper script
./run-docker.sh raspberry-pi
```

### Method 2: Using Docker Compose

```bash
# Allow X11 connections
xhost +local:docker

# Build and run
docker-compose --profile raspberry-pi up --build
```

### Method 3: Manual Docker Run

```bash
# Build the ARM64 image
docker build -f Dockerfile.arm64 -t generations:arm64 .

# Run the container
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --device /dev/dri:/dev/dri \
  --device /dev/vchiq:/dev/vchiq \
  generations:arm64
```

## Performance Tips

### 1. Overclocking (Raspberry Pi 4)

**Warning:** Overclocking may void warranty and requires adequate cooling.

```bash
sudo nano /boot/config.txt

# Add these lines for mild overclocking
over_voltage=2
arm_freq=1750
gpu_freq=600

# Ensure you have adequate cooling!
```

### 2. Use Raspberry Pi OS 64-bit

Ensure you're running the 64-bit version of Raspberry Pi OS for better performance:

```bash
uname -m
# Should output: aarch64
```

If it shows `armv7l`, you're on 32-bit. Download and install the 64-bit version from [raspberrypi.com/software](https://www.raspberrypi.com/software/).

### 3. Close Unnecessary Applications

```bash
# Stop desktop environment if running headless
sudo systemctl stop lightdm

# Run GENERATIONS
./run-docker.sh raspberry-pi

# Restart desktop when done
sudo systemctl start lightdm
```

### 4. Monitor Temperature

```bash
# Install monitoring tools
sudo apt-get install -y stress

# Watch temperature while running
watch -n 1 vcgencmd measure_temp
```

Keep temperature below 80°C for optimal performance. If it's too hot:
- Improve cooling (add heatsinks/fan)
- Reduce overclocking
- Lower gpu_mem

## Troubleshooting

### Docker Build is Slow

ARM64 builds take longer than x86_64. On a Raspberry Pi 4:
- First build: 15-30 minutes
- Subsequent builds: 5-10 minutes (with cache)

Be patient and ensure adequate cooling during build.

### "Cannot open display" Error

```bash
# Allow X11 connections
xhost +local:docker

# Check DISPLAY variable
echo $DISPLAY
# Should show something like :0

# If empty, set it
export DISPLAY=:0
```

### VideoCore Device Not Found

```bash
# Check if device exists
ls -la /dev/vchiq

# If not found, load the module
sudo modprobe vchiq

# Add to /etc/modules for automatic loading
echo "vchiq" | sudo tee -a /etc/modules
```

### Out of Memory During Build

If you have 2GB RAM or less:

```bash
# Increase swap size
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
# Set: CONF_SWAPSIZE=2048

sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```

### Vulkan Not Available

```bash
# Reinstall Mesa drivers
sudo apt-get install --reinstall mesa-vulkan-drivers

# Check Vulkan
vulkaninfo

# If vulkaninfo is not found:
sudo apt-get install vulkan-tools
```

### Performance is Poor

1. **Check CPU throttling:**
   ```bash
   vcgencmd get_throttled
   # 0x0 is good, anything else indicates throttling
   ```

2. **Monitor resources:**
   ```bash
   htop
   ```

3. **Reduce simulation size** (if applicable in the application)

4. **Ensure adequate power supply** (Use official Raspberry Pi power supply)

## Benchmarks

Approximate build times on different Raspberry Pi models:

| Model | RAM | Build Time | Runtime Performance |
|-------|-----|------------|-------------------|
| Pi 4 2GB | 2GB | ~25 min | Good with cooling |
| Pi 4 4GB | 4GB | ~20 min | Good |
| Pi 4 8GB | 8GB | ~20 min | Best for Pi 4 |
| Pi 5 4GB | 4GB | ~12 min | Excellent |
| Pi 5 8GB | 8GB | ~12 min | Excellent |

*Times are approximate and depend on SD card speed, cooling, and other factors.*

## Remote Access

### Running Headless with VNC

```bash
# Enable VNC
sudo raspi-config
# Interface Options -> VNC -> Enable

# Use VNC viewer to connect
# Then run GENERATIONS normally
```

### Running Headless with X11 Forwarding

```bash
# On your local machine (with X server):
ssh -X pi@raspberry-pi-address

# On the Pi:
./run-docker.sh raspberry-pi
```

## Additional Resources

- [Raspberry Pi Vulkan Documentation](https://www.raspberrypi.com/documentation/computers/vulkan.html)
- [Docker on Raspberry Pi](https://docs.docker.com/engine/install/debian/)
- [Raspberry Pi Forums](https://forums.raspberrypi.com/)

## Getting Help

If you encounter issues specific to Raspberry Pi:

1. Check the main [DOCKER.md](DOCKER.md) troubleshooting section
2. Verify Vulkan works outside Docker: `vulkaninfo`
3. Check system resources: `htop`, `vcgencmd measure_temp`
4. Open an issue on GitHub with:
   - Pi model and RAM
   - OS version: `cat /etc/os-release`
   - Kernel version: `uname -a`
   - Error messages or logs
