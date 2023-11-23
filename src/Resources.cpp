#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Resources.h"

Resources::Resources(VulkanMechanics& mechanics, Pipelines& pipelines)
    : commands{mechanics.queues.familyIndices},
      pushConstants{},
      depthImage{mechanics.swapchain.extent, CE::Image::findDepthFormat()},
      msaaImage{mechanics.swapchain.extent, mechanics.swapchain.imageFormat},
      shaderStorage{commands.singularCommandBuffer, commands.pool,
                    mechanics.queues.graphics, world.grid.cells,
                    world.grid.pointCount},
      sampler{commands.singularCommandBuffer, commands.pool,
              mechanics.queues.graphics},
      storageImage{mechanics.swapchain.images},
      world{commands.singularCommandBuffer, commands.pool,
            mechanics.queues.graphics} {
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "constructing Resources");
  CE::Descriptor::createSetLayout(CE::Descriptor::setLayoutBindings);
  CE::Descriptor::createPool();
  CE::Descriptor::allocateSets();
  CE::Descriptor::createSets();
}

Resources::~Resources() {
  Log::text("{ /// }", "destructing Resources");
}

void Resources::ShaderStorage::create(VkCommandBuffer& commandBuffer,
                                      const VkCommandPool& commandPool,
                                      const VkQueue& queue,
                                      const auto& object,

                                      const size_t quantity) {
  Log::text("{ 101 }", "Shader Storage Buffers");

  // Create a staging buffer used to upload data to the gpu
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(World::Cell) * quantity;

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void* data;
  vkMapMemory(CE::Device::baseDevice->logical, stagingResources.memory, 0,
              bufferSize, 0, &data);
  std::memcpy(data, object.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(CE::Device::baseDevice->logical, stagingResources.memory);

  CE::Buffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferIn);
  CE::Buffer::copy(stagingResources.buffer, bufferIn.buffer, bufferSize,
                   commandBuffer, commandPool, queue);

  CE::Buffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferOut);
  CE::Buffer::copy(stagingResources.buffer, bufferOut.buffer, bufferSize,
                   commandBuffer, commandPool, queue);
}

void Resources::UniformBuffer::create() {
  Log::text("{ 101 }", MAX_FRAMES_IN_FLIGHT, "Uniform Buffers");
  VkDeviceSize bufferSize = sizeof(World::UniformBufferObject);

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     buffer);

  vkMapMemory(CE::Device::baseDevice->logical, buffer.memory, 0, bufferSize, 0,
              &buffer.mapped);
}

void Resources::UniformBuffer::update(World& world, const VkExtent2D extent) {
  object.light = world.light.position;
  object.gridXY = glm::vec2(static_cast<uint32_t>(world.grid.size.x),
                            static_cast<uint32_t>(world.grid.size.y));
  object.waterThreshold = 0.1f;
  object.cellSize = world.grid.initialCellSize;
  object.model = world.camera.setModel();
  object.view = world.camera.setView();
  object.projection = world.camera.setProjection(extent);

  std::memcpy(buffer.mapped, &object, sizeof(object));
}

void Resources::Commands::recordComputeCommandBuffer(
    Resources& resources,
    Pipelines& pipelines,
    const uint32_t imageIndex) {
  VkCommandBuffer commandBuffer = this->compute[imageIndex];

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelines.config.getPipelineObjectByName("Engine"));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelines.compute.layout, 0, 1,
                          &CE::Descriptor::sets[imageIndex], 0, nullptr);

  resources.pushConstants.setData(resources.world.time.passedHours);

  vkCmdPushConstants(
      commandBuffer, pipelines.compute.layout,
      resources.pushConstants.shaderStage, resources.pushConstants.offset,
      resources.pushConstants.size, resources.pushConstants.data.data());

  const std::array<uint32_t, 3>& workGroups =
      pipelines.config.getWorkGroupsByName("Engine");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[0], workGroups[2]);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

void Resources::Commands::recordGraphicsCommandBuffer(
    CE::Swapchain& swapchain,
    Resources& resources,
    Pipelines& pipelines,
    const uint32_t imageIndex) {
  VkCommandBuffer commandBuffer = this->graphics[imageIndex];

  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  CE::VULKAN_RESULT(vkBeginCommandBuffer, commandBuffer, &beginInfo);

  std::array<VkClearValue, 2> clearValues{
      VkClearValue{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
      VkClearValue{.depthStencil = {1.0f, 0}}};

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = pipelines.render.renderPass,
      .framebuffer = swapchain.framebuffers[imageIndex],
      .renderArea = {.offset = {0, 0}, .extent = swapchain.extent},
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()};

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport{.x = 0.0f,
                      .y = 0.0f,
                      .width = static_cast<float>(swapchain.extent.width),
                      .height = static_cast<float>(swapchain.extent.height),
                      .minDepth = 0.0f,
                      .maxDepth = 1.0f};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{.offset = {0, 0}, .extent = swapchain.extent};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelines.graphics.layout, 0, 1,
                          &CE::Descriptor::sets[imageIndex], 0, nullptr);

  // Pipeline 1
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Cells"));
  VkDeviceSize offsets[]{0};

  VkDeviceSize offsets0[]{0, 0};

  VkBuffer currentShaderStorageBuffer[] = {
      resources.shaderStorage.bufferIn.buffer,
      resources.shaderStorage.bufferOut.buffer};

  VkBuffer vertexBuffers0[] = {currentShaderStorageBuffer[imageIndex],
                               resources.world.cube.vertexBuffer.buffer};

  vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers0, offsets0);
  vkCmdDraw(commandBuffer,
            static_cast<uint32_t>(resources.world.cube.allVertices.size()),
            resources.world.grid.size.x * resources.world.grid.size.y, 0, 0);

  // Landscape
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Landscape"));
  VkBuffer vertexBuffers1[] = {resources.world.grid.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers1, offsets);
  vkCmdBindIndexBuffer(commandBuffer, resources.world.grid.indexBuffer.buffer,
                       0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world.grid.indices.size()),
                   1, 0, 0, 0);

  // Landscape Wireframe
  // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                  _pipelines.graphics.landscapeWireframe);
  // vkCmdDrawIndexed(commandBuffer,
  //                 static_cast<uint32_t>(world.landscape.indices.size()), 1,
  //                 0, 0, 0);

  // Pipeline 3
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Water"));
  VkBuffer vertexBuffers[] = {resources.world.rectangle.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer,
                       resources.world.rectangle.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(
      commandBuffer,
      static_cast<uint32_t>(resources.world.rectangle.indices.size()), 1, 0, 0,
      0);

  // Pipeline 4
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Texture"));
  vkCmdDrawIndexed(
      commandBuffer,
      static_cast<uint32_t>(resources.world.rectangle.indices.size()), 1, 0, 0,
      0);
  vkCmdEndRenderPass(commandBuffer);

  //       This is part of an image memory barrier (i.e., vkCmdPipelineBarrier
  //       with the VkImageMemoryBarrier parameter set)

  swapchain.images[imageIndex].transitionLayout(
      commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      /* -> */ VK_IMAGE_LAYOUT_GENERAL);

  // vkDeviceWaitIdle(_mainDevice.logical);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    pipelines.config.getPipelineObjectByName("PostFX"));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelines.compute.layout, 0, 1,
                          &CE::Descriptor::sets[imageIndex], 0, nullptr);

  resources.pushConstants.setData(resources.world.time.passedHours);
  vkCmdPushConstants(
      commandBuffer, pipelines.compute.layout,
      resources.pushConstants.shaderStage, resources.pushConstants.offset,
      resources.pushConstants.size, resources.pushConstants.data.data());

  const std::array<uint32_t, 3>& workGroups =
      pipelines.config.getWorkGroupsByName("PostFX");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[1], workGroups[2]);

  swapchain.images[imageIndex].transitionLayout(
      commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_GENERAL,
      /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

Resources::Commands::Commands(const CE::Queues::FamilyIndices& familyIndices) {
  createPool(familyIndices);
  createBuffers(graphics);
  createBuffers(compute);
}

Resources::DepthImage::DepthImage(const VkExtent2D extent,
                                  const VkFormat format) {
  createResources(extent, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  VK_IMAGE_ASPECT_DEPTH_BIT);
}

Resources::MultisamplingImage::MultisamplingImage(const VkExtent2D extent,
                                                  const VkFormat format) {
  createResources(extent, format,
                  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                  VK_IMAGE_ASPECT_COLOR_BIT);
}
