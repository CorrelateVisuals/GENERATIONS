#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Forward declarations to avoid pulling in Vulkan headers
namespace CE {
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr size_t NUM_DESCRIPTORS = 5;
}  // namespace CE

/**
 * @brief Abstract Rendering Interface
 * 
 * This namespace provides API-agnostic interfaces for rendering operations.
 * The actual implementation (Vulkan, OpenGL, etc.) is hidden behind these interfaces.
 */
namespace RenderInterface {

// ==============================================================================
// Core Types
// ==============================================================================

using DeviceHandle = void*;
using BufferHandle = void*;
using ImageHandle = void*;
using PipelineHandle = void*;
using CommandBufferHandle = void*;

enum class ResourceType {
  DEPTH_IMAGE,
  MULTISAMPLE_IMAGE,
  TEXTURE_IMAGE,
  STORAGE_IMAGE
};

enum class BufferUsage {
  VERTEX,
  INDEX,
  UNIFORM,
  STORAGE,
  TRANSFER_SRC,
  TRANSFER_DST
};

enum class MemoryProperty {
  DEVICE_LOCAL,
  HOST_VISIBLE,
  HOST_COHERENT
};

enum class ImageFormat {
  UNDEFINED,
  R8G8B8A8_SRGB,
  R8G8B8A8_UNORM,
  B8G8R8A8_SRGB,
  D32_SFLOAT,
  D24_UNORM_S8_UINT
};

enum class PipelineType {
  GRAPHICS,
  COMPUTE
};

struct Extent2D {
  uint32_t width{0};
  uint32_t height{0};
};

struct Extent3D {
  uint32_t width{0};
  uint32_t height{0};
  uint32_t depth{1};
};

// ==============================================================================
// Abstract Device Interface
// ==============================================================================

/**
 * @brief Abstract device interface for GPU operations
 * 
 * Provides device management independent of the underlying graphics API.
 */
class IDevice {
 public:
  virtual ~IDevice() = default;

  // Device information
  virtual const char* getDeviceName() const = 0;
  virtual uint32_t getMaxSampleCount() const = 0;
  
  // Device capabilities
  virtual bool supportsTessellation() const = 0;
  virtual bool supportsGeometryShader() const = 0;
  virtual bool supportsComputeShader() const = 0;

  // Wait for device to be idle
  virtual void waitIdle() const = 0;
};

// ==============================================================================
// Abstract Buffer Interface
// ==============================================================================

/**
 * @brief Abstract buffer interface for GPU memory management
 */
class IBuffer {
 public:
  virtual ~IBuffer() = default;

  // Buffer operations
  virtual void* map() = 0;
  virtual void unmap() = 0;
  virtual void copyFrom(const void* data, size_t size, size_t offset = 0) = 0;
  virtual void copyTo(IBuffer& dst, size_t size) = 0;

  // Buffer properties
  virtual size_t getSize() const = 0;
  virtual BufferUsage getUsage() const = 0;
};

// ==============================================================================
// Abstract Image Interface
// ==============================================================================

/**
 * @brief Abstract image/texture interface
 */
class IImage {
 public:
  virtual ~IImage() = default;

  // Image operations
  virtual void transitionLayout(void* commandBuffer, int oldLayout, int newLayout) = 0;
  virtual void loadFromFile(const std::string& path) = 0;

  // Image properties
  virtual Extent2D getExtent() const = 0;
  virtual ImageFormat getFormat() const = 0;
  virtual uint32_t getMipLevels() const = 0;
};

// ==============================================================================
// Abstract Pipeline Interface
// ==============================================================================

/**
 * @brief Abstract graphics/compute pipeline interface
 */
class IPipeline {
 public:
  virtual ~IPipeline() = default;

  virtual PipelineType getType() const = 0;
  virtual const std::vector<std::string>& getShaders() const = 0;
  
  // Bind pipeline for rendering
  virtual void bind(CommandBufferHandle commandBuffer) = 0;
};

// ==============================================================================
// Abstract Command Buffer Interface
// ==============================================================================

/**
 * @brief Abstract command buffer for recording GPU commands
 */
class ICommandBuffer {
 public:
  virtual ~ICommandBuffer() = default;

  // Command recording
  virtual void begin() = 0;
  virtual void end() = 0;
  virtual void reset() = 0;

  // Drawing commands
  virtual void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
                   uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
  virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                          uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                          uint32_t firstInstance = 0) = 0;
  virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY,
                       uint32_t groupCountZ) = 0;

  // State binding
  virtual void bindPipeline(IPipeline& pipeline) = 0;
  virtual void bindVertexBuffer(IBuffer& buffer, uint32_t binding = 0) = 0;
  virtual void bindIndexBuffer(IBuffer& buffer) = 0;
};

// ==============================================================================
// Abstract Synchronization Interface
// ==============================================================================

/**
 * @brief Abstract synchronization primitives
 */
class ISync {
 public:
  virtual ~ISync() = default;

  virtual void waitForFence(uint32_t frameIndex) = 0;
  virtual void resetFence(uint32_t frameIndex) = 0;
  virtual uint32_t getCurrentFrame() const = 0;
};

// ==============================================================================
// Abstract Swapchain Interface
// ==============================================================================

/**
 * @brief Abstract swapchain for presentation
 */
class ISwapchain {
 public:
  virtual ~ISwapchain() = default;

  virtual Extent2D getExtent() const = 0;
  virtual ImageFormat getImageFormat() const = 0;
  virtual uint32_t getImageCount() const = 0;
  virtual uint32_t acquireNextImage() = 0;
  virtual void present(uint32_t imageIndex) = 0;
};

// ==============================================================================
// Factory Interface for Creating Rendering Objects
// ==============================================================================

/**
 * @brief Factory for creating rendering API objects
 * 
 * This factory allows creation of rendering objects without knowing
 * the underlying implementation (Vulkan, OpenGL, etc.)
 */
class IRenderFactory {
 public:
  virtual ~IRenderFactory() = default;

  // Device creation
  virtual std::unique_ptr<IDevice> createDevice() = 0;

  // Resource creation
  virtual std::unique_ptr<IBuffer> createBuffer(size_t size, BufferUsage usage,
                                               MemoryProperty properties) = 0;
  virtual std::unique_ptr<IImage> createImage(Extent2D extent, ImageFormat format,
                                             ResourceType type) = 0;

  // Pipeline creation
  virtual std::unique_ptr<IPipeline> createGraphicsPipeline(
      const std::vector<std::string>& shaders) = 0;
  virtual std::unique_ptr<IPipeline> createComputePipeline(
      const std::string& shader) = 0;

  // Command buffer creation
  virtual std::unique_ptr<ICommandBuffer> createCommandBuffer() = 0;

  // Synchronization
  virtual std::unique_ptr<ISync> createSyncObjects() = 0;

  // Swapchain
  virtual std::unique_ptr<ISwapchain> createSwapchain() = 0;
};

}  // namespace RenderInterface
