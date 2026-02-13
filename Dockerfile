# Multi-stage Dockerfile for GENERATIONS with Vulkan/GPU support
# Supports both AMD and NVIDIA GPUs on x86_64 systems

ARG BASE_IMAGE=ubuntu:22.04
FROM ${BASE_IMAGE} as builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    python3 \
    python3-pip \
    libglfw3-dev \
    libglm-dev \
    vulkan-tools \
    libvulkan-dev \
    vulkan-validationlayers \
    spirv-tools \
    glslang-tools \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libx11-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN cmake -S . -B build -Wno-dev --log-level=NOTICE && \
    cmake --build build --parallel $(nproc)

# Runtime stage
FROM ${BASE_IMAGE} as runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libglfw3 \
    libvulkan1 \
    vulkan-tools \
    libx11-6 \
    libxrandr2 \
    libxinerama1 \
    libxcursor1 \
    libxi6 \
    mesa-vulkan-drivers \
    && rm -rf /var/lib/apt/lists/*

# Copy built application from builder
WORKDIR /app
COPY --from=builder /app/bin/CapitalEngine /app/bin/
COPY --from=builder /app/shaders/*.spv /app/shaders/
COPY --from=builder /app/assets /app/assets

# Set display environment variable (can be overridden)
ENV DISPLAY=:0

# Run the application
CMD ["/app/bin/CapitalEngine"]
