#pragma once
#include "vulkan/vulkan.h"

#include "BaseClasses.h"
#include "Mechanics.h"
#include "Pipelines.h"
#include "World.h"

#include <array>
#include <cstring>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class Resources {
 public:
  Resources(VulkanMechanics& mechanics, Pipelines& pipelines);
  ~Resources();

  struct CommandResources : public CE::CommandBuffers {
    CommandResources(const CE::Queues::FamilyIndices& family_indices);
    void record_compute_command_buffer(Resources& resources,
                                       Pipelines& pipelines,
                                       const uint32_t image_index) override;
    void record_graphics_command_buffer(CE::Swapchain& swapchain,
                                        Resources& resources,
                                        Pipelines& pipelines,
                                        const uint32_t image_index) override;
  };

  class UniformBuffer : public CE::Descriptor {
   public:
    UniformBuffer(World::UniformBufferObject& u);
    void update(World& _world, const VkExtent2D extent);

   private:
    CE::Buffer buffer;
    World::UniformBufferObject& _ubo;
    void create_buffer();
    void create_descriptor_write();
  };

  class StorageBuffer : public CE::Descriptor {
   public:
    CE::Buffer bufferIn;
    CE::Buffer bufferOut;

    StorageBuffer(const CE::CommandInterface& commandData,
                  const auto& object,
                  const size_t quantity);

   private:
    void create(const CE::CommandInterface& commandData,
                const auto& object,
                const size_t quantity);
    void create_descriptor_write(const size_t quantity);
  };

  class ImageSampler : public CE::Descriptor {
   public:
    ImageSampler(const CE::CommandInterface& commandData,
                 const std::string& texturePath);

   private:
    void create_descriptor_write();
    CE::Image textureImage;
  };

  class StorageImage : public CE::Descriptor {
   public:
    StorageImage(std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
    void create_descriptor_write(
        std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images);
  };
  // GPU Interface
  CommandResources _commands;
  CE::CommandInterface _commandInterface;
  CE::PushConstants _pushConstant;

  // Scene
  World _world;  // World objects, light, _camera

  // Images
  CE::Image _depthImage;
  CE::Image _msaaImage;

  // Descriptors
  UniformBuffer _uniform;        // UniformParameters _world details
  StorageBuffer _shaderStorage;  // FeedbackLoop compute shader

  // Image Descriptors
  ImageSampler sampler;       // Texture vertex shader
  StorageImage storageImage;  //  PostFX compute shader
};
