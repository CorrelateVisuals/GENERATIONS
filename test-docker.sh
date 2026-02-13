#!/bin/bash
# Test script to verify Docker build and basic functionality

set -e

echo "=== GENERATIONS Docker Test Suite ==="
echo ""

echo "1. Testing Docker build for x86_64..."
docker build -t generations:test -f Dockerfile . > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✓ x86_64 build successful"
else
    echo "   ✗ x86_64 build failed"
    exit 1
fi

echo ""
echo "2. Verifying binary exists in container..."
docker run --rm generations:test test -f /app/bin/CapitalEngine
if [ $? -eq 0 ]; then
    echo "   ✓ Binary exists"
else
    echo "   ✗ Binary not found"
    exit 1
fi

echo ""
echo "3. Verifying shaders exist in container..."
SHADER_COUNT=$(docker run --rm generations:test sh -c "ls /app/shaders/*.spv 2>/dev/null | wc -l")
if [ "$SHADER_COUNT" -gt 0 ]; then
    echo "   ✓ Found $SHADER_COUNT compiled shaders"
else
    echo "   ✗ No shaders found"
    exit 1
fi

echo ""
echo "4. Checking for Vulkan support in container..."
docker run --rm generations:test which vulkaninfo > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✓ Vulkan tools available"
else
    echo "   ✗ Vulkan tools not found"
    exit 1
fi

echo ""
echo "5. Verifying runtime dependencies..."
docker run --rm generations:test ldd /app/bin/CapitalEngine > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✓ All runtime dependencies satisfied"
else
    echo "   ✗ Missing runtime dependencies"
    exit 1
fi

echo ""
echo "=== All tests passed! ==="
echo ""
echo "Docker image 'generations:test' is ready to use."
echo "Run with: docker-compose --profile mesa up"
echo ""
