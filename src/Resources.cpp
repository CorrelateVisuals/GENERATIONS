#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Resources.h"

Resources::Resources(VulkanMechanics& mechanics)
    : _mechanics(mechanics),
      pushConstants{},
      textureImage{},
      command{mechanics.queues.familyIndices},
      msaaImage{},
      shaderStorage{sizeof(World::Cell) * world.grid.size.x *
                    world.grid.size.y},
      sampler{textureImage},
      storageImage{mechanics.swapchain.images} {
  Log::text("{ /// }", "constructing Resources");

  CE::Descriptor::createSetLayout(CE::Descriptor::setLayoutBindings);
  msaaImage.createColorResources(mechanics.swapchain.extent,
                                 mechanics.swapchain.imageFormat);
  depthImage.createDepthResources(mechanics.swapchain.extent,
                                  CE::Image::findDepthFormat());
}

Resources::~Resources() {
  Log::text("{ /// }", "destructing Resources");
}

void Resources::setupResources(Pipelines& _pipelines) {
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "Setup Resources");

  textureImage.loadTexture(
      Lib::path("assets/Avatar.PNG"), VK_FORMAT_R8G8B8A8_SRGB,
      command.singularCommandBuffer, command.pool, _mechanics.queues.graphics);
  textureImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);
  textureImage.createSampler();

  createFramebuffers(_pipelines);
  createShaderStorageBuffers();
  createUniformBuffers();
  createVertexBuffers(vertexBuffers);

  createDescriptorPool();  // moved
  allocateDescriptorSets();
  createDescriptorSets();

  createCommandBuffers(command.graphics, MAX_FRAMES_IN_FLIGHT);
  createCommandBuffers(command.compute, MAX_FRAMES_IN_FLIGHT);
}

void Resources::createFramebuffers(Pipelines& _pipelines) {
  Log::text("{ 101 }", "Frame Buffers:", _mechanics.swapchain.images.size());

  _mechanics.swapchain.framebuffers.resize(_mechanics.swapchain.images.size());

  Log::text(Log::Style::charLeader,
            "attachments: msaaImage., depthImage, swapchain imageViews");
  for (uint_fast8_t i = 0; i < _mechanics.swapchain.images.size(); i++) {
    std::vector<VkImageView> attachments{msaaImage.view, depthImage.view,
                                         _mechanics.swapchain.images[i].view};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = _pipelines.render.renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = _mechanics.swapchain.extent.width,
        .height = _mechanics.swapchain.extent.height,
        .layers = 1};

    CE::VULKAN_RESULT(vkCreateFramebuffer, _mechanics.mainDevice.logical,
                      &framebufferInfo, nullptr,
                      &_mechanics.swapchain.framebuffers[i]);
  }
}

void Resources::createShaderStorageBuffers() {
  Log::text("{ 101 }", "Shader Storage Buffers");

  std::vector<World::Cell> cells = world.initializeGrid();

  // Create a staging buffer used to upload data to the gpu
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize =
      sizeof(World::Cell) * world.grid.size.x * world.grid.size.y;

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void* data;
  vkMapMemory(_mechanics.mainDevice.logical, stagingResources.memory, 0,
              bufferSize, 0, &data);
  std::memcpy(data, cells.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(_mechanics.mainDevice.logical, stagingResources.memory);

  CE::Buffer::create(
      static_cast<VkDeviceSize>(bufferSize),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorage.bufferIn);
  CE::Buffer::copy(stagingResources.buffer, shaderStorage.bufferIn.buffer,
                   bufferSize, command.singularCommandBuffer, command.pool,
                   _mechanics.queues.graphics);

  CE::Buffer::create(
      static_cast<VkDeviceSize>(bufferSize),
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorage.bufferOut);
  CE::Buffer::copy(stagingResources.buffer, shaderStorage.bufferOut.buffer,
                   bufferSize, command.singularCommandBuffer, command.pool,
                   _mechanics.queues.graphics);
}

void Resources::createUniformBuffers() {
  Log::text("{ 101 }", MAX_FRAMES_IN_FLIGHT, "Uniform Buffers");
  VkDeviceSize bufferSize = sizeof(World::UniformBufferObject);

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniform.buffer);

  vkMapMemory(_mechanics.mainDevice.logical, uniform.buffer.memory, 0,
              bufferSize, 0, &uniform.buffer.mapped);
}

// void Resources::createDescriptorSetLayout(
//     const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings) {
//   Log::text("{ |=| }", "Descriptor Set Layout:", layoutBindings.size(),
//             "bindings");
//   for (const VkDescriptorSetLayoutBinding& item : layoutBindings) {
//     Log::text("{ ", item.binding, " }",
//               Log::getDescriptorTypeString(item.descriptorType));
//     Log::text(Log::Style::charLeader,
//               Log::getShaderStageString(item.stageFlags));
//   }
//
//   VkDescriptorSetLayoutCreateInfo layoutInfo{
//       .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//       .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
//       .pBindings = layoutBindings.data()};
//
//   CE::VULKAN_RESULT(vkCreateDescriptorSetLayout,
//   _mechanics.mainDevice.logical,
//                     &layoutInfo, nullptr, &CE::Descriptor::setLayout);
// }

void Resources::createDescriptorPool() {
  Log::text("{ |=| }", "Descriptor Pool");
  for (size_t i = 0; i < CE::Descriptor::poolSizes.size(); i++) {
    Log::text(Log::Style::charLeader,
              Log::getDescriptorTypeString(CE::Descriptor::poolSizes[i].type));
  }
  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
      .poolSizeCount = static_cast<uint32_t>(CE::Descriptor::poolSizes.size()),
      .pPoolSizes = CE::Descriptor::poolSizes.data()};

  CE::VULKAN_RESULT(vkCreateDescriptorPool, _mechanics.mainDevice.logical,
                    &poolInfo, nullptr, &CE::Descriptor::pool);
}

void Resources::allocateDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             CE::Descriptor::setLayout);
  VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = CE::Descriptor::pool,
      .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
      .pSetLayouts = layouts.data()};

  CE::Descriptor::sets.resize(MAX_FRAMES_IN_FLIGHT);
  CE::VULKAN_RESULT(vkAllocateDescriptorSets, _mechanics.mainDevice.logical,
                    &allocateInfo, CE::Descriptor::sets.data());
}

void Resources::createCommandBuffers(
    std::vector<VkCommandBuffer>& commandBuffers,
    const int size) {
  Log::text("{ cmd }", "Command Buffers:", size);

  commandBuffers.resize(size);
  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command.pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())};

  CE::VULKAN_RESULT(vkAllocateCommandBuffers, _mechanics.mainDevice.logical,
                    &allocateInfo, commandBuffers.data());
}

void Resources::createVertexBuffers(
    const std::unordered_map<Geometry*, VkVertexInputRate>& buffers) {
  for (const auto& resource : buffers) {
    Geometry* currentGeometry = resource.first;

    if (resource.second == VK_VERTEX_INPUT_RATE_INSTANCE) {
      createVertexBuffer(currentGeometry->vertexBuffer,
                         currentGeometry->uniqueVertices);
      createIndexBuffer(currentGeometry->indexBuffer, currentGeometry->indices);
    } else if (resource.second == VK_VERTEX_INPUT_RATE_VERTEX) {
      createVertexBuffer(currentGeometry->vertexBuffer,
                         currentGeometry->allVertices);
    }
  }
};

void Resources::createVertexBuffer(CE::Buffer& buffer, const auto& vertices) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void* data;
  vkMapMemory(_mechanics.mainDevice.logical, stagingResources.memory, 0,
              bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(_mechanics.mainDevice.logical, stagingResources.memory);

  CE::Buffer::create(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer);

  CE::Buffer::copy(stagingResources.buffer, buffer.buffer, bufferSize,
                   command.singularCommandBuffer, command.pool,
                   _mechanics.queues.graphics);
}

void Resources::createIndexBuffer(CE::Buffer& buffer, const auto& indices) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void* data;
  vkMapMemory(_mechanics.mainDevice.logical, stagingResources.memory, 0,
              bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(_mechanics.mainDevice.logical, stagingResources.memory);

  CE::Buffer::create(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer);

  CE::Buffer::copy(stagingResources.buffer, buffer.buffer, bufferSize,
                   command.singularCommandBuffer, command.pool,
                   _mechanics.queues.graphics);
}

void Resources::setPushConstants() {
  pushConstants.data = {world.time.passedHours};
}

void Resources::createDescriptorSets() {
  Log::text("{ |=| }", "Descriptor Sets");

  for (uint_fast8_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo uniformBufferInfo{
        .buffer = uniform.buffer.buffer,
        .offset = 0,
        .range = sizeof(World::UniformBufferObject)};

    VkDescriptorBufferInfo storageBufferInfoLastFrame{
        .buffer = shaderStorage.bufferIn.buffer,
        .offset = 0,
        .range = sizeof(World::Cell) * world.grid.size.x * world.grid.size.y};

    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{
        .buffer = shaderStorage.bufferOut.buffer,
        .offset = 0,
        .range = sizeof(World::Cell) * world.grid.size.x * world.grid.size.y};

    VkDescriptorImageInfo imageInfo{
        .sampler = textureImage.sampler,
        .imageView = textureImage.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    VkDescriptorImageInfo swapchainImageInfo{
        .sampler = VK_NULL_HANDLE,
        .imageView = _mechanics.swapchain.images[i].view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

    std::vector<VkWriteDescriptorSet> descriptorWrites{
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = CE::Descriptor::sets[i],
         .dstBinding = 0,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &uniformBufferInfo},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = CE::Descriptor::sets[i],
         .dstBinding = static_cast<uint32_t>(i ? 2 : 1),
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .pBufferInfo = &storageBufferInfoLastFrame},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = CE::Descriptor::sets[i],
         .dstBinding = static_cast<uint32_t>(i ? 1 : 2),
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .pBufferInfo = &storageBufferInfoCurrentFrame},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = CE::Descriptor::sets[i],
         .dstBinding = 3,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &imageInfo},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = CE::Descriptor::sets[i],
         .dstBinding = 4,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .pImageInfo = &swapchainImageInfo},
    };

    vkUpdateDescriptorSets(_mechanics.mainDevice.logical,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void Resources::updateUniformBuffer(uint32_t currentImage) {
  World::UniformBufferObject uniformObject =
      world.updateUniforms(_mechanics.swapchain.extent);
  std::memcpy(uniform.buffer.mapped, &uniformObject, sizeof(uniformObject));
}

void Resources::recordComputeCommandBuffer(VkCommandBuffer commandBuffer,
                                           Pipelines& _pipelines) {
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  // vkCmdPipelineBarrier(
  //     command.compute[_mechanics.syncObjects.currentFrame],
  //     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
  //     VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 0,
  //     nullptr);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    _pipelines.config.getPipelineObjectByName("Engine"));

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelines.compute.layout,
      0, 1, &CE::Descriptor::sets[_mechanics.syncObjects.currentFrame], 0,
      nullptr);

  setPushConstants();
  vkCmdPushConstants(commandBuffer, _pipelines.compute.layout,
                     pushConstants.shaderStage, pushConstants.offset,
                     pushConstants.size, pushConstants.data.data());

  const std::array<uint32_t, 3>& workGroups =
      _pipelines.config.getWorkGroupsByName("Engine");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[0], workGroups[2]);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

void Resources::recordGraphicsCommandBuffer(VkCommandBuffer commandBuffer,
                                            uint32_t imageIndex,
                                            Pipelines& _pipelines) {
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  CE::VULKAN_RESULT(vkBeginCommandBuffer, commandBuffer, &beginInfo);

  std::vector<VkClearValue> clearValues{{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                                        {.depthStencil = {1.0f, 0}}};

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = _pipelines.render.renderPass,
      .framebuffer = _mechanics.swapchain.framebuffers[imageIndex],
      .renderArea = {.offset = {0, 0}, .extent = _mechanics.swapchain.extent},
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()};

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(_mechanics.swapchain.extent.width),
      .height = static_cast<float>(_mechanics.swapchain.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{.offset = {0, 0}, .extent = _mechanics.swapchain.extent};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      _pipelines.graphics.layout, 0, 1,
      &CE::Descriptor::sets[_mechanics.syncObjects.currentFrame], 0, nullptr);

  // Pipeline 1
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _pipelines.config.getPipelineObjectByName("Cells"));
  VkDeviceSize offsets[]{0};

  VkDeviceSize offsets0[]{0, 0};

  VkBuffer currentShaderStorageBuffer[] = {shaderStorage.bufferIn.buffer,
                                           shaderStorage.bufferOut.buffer};

  VkBuffer vertexBuffers0[] = {
      currentShaderStorageBuffer[_mechanics.syncObjects.currentFrame],
      world.cube.vertexBuffer.buffer};

  vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers0, offsets0);
  vkCmdDraw(commandBuffer, static_cast<uint32_t>(world.cube.allVertices.size()),
            world.grid.size.x * world.grid.size.y, 0, 0);

  // Landscape
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _pipelines.config.getPipelineObjectByName("Landscape"));
  VkBuffer vertexBuffers1[] = {world.landscape.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers1, offsets);
  vkCmdBindIndexBuffer(commandBuffer, world.landscape.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(world.landscape.indices.size()), 1, 0,
                   0, 0);

  // Landscape Wireframe
  // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                  _pipelines.graphics.landscapeWireframe);
  // vkCmdDrawIndexed(commandBuffer,
  //                 static_cast<uint32_t>(world.landscape.indices.size()), 1,
  //                 0, 0, 0);

  // Pipeline 3
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _pipelines.config.getPipelineObjectByName("Water"));
  VkBuffer vertexBuffers[] = {world.rectangle.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, world.rectangle.indexBuffer.buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(world.rectangle.indices.size()), 1, 0,
                   0, 0);

  // Pipeline 4
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _pipelines.config.getPipelineObjectByName("Texture"));
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(world.rectangle.indices.size()), 1, 0,
                   0, 0);
  vkCmdEndRenderPass(commandBuffer);

  //       This is part of an image memory barrier (i.e., vkCmdPipelineBarrier
  //       with the VkImageMemoryBarrier parameter set)

  _mechanics.swapchain.images[_mechanics.syncObjects.currentFrame]
      .transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                        /* -> */ VK_IMAGE_LAYOUT_GENERAL);

  // vkDeviceWaitIdle(_mechanics.mainDevice.logical);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    _pipelines.config.getPipelineObjectByName("PostFX"));

  vkCmdBindDescriptorSets(
      commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelines.compute.layout,
      0, 1, &CE::Descriptor::sets[_mechanics.syncObjects.currentFrame], 0,
      nullptr);

  setPushConstants();
  vkCmdPushConstants(commandBuffer, _pipelines.compute.layout,
                     pushConstants.shaderStage, pushConstants.offset,
                     pushConstants.size, pushConstants.data.data());

  const std::array<uint32_t, 3>& workGroups =
      _pipelines.config.getWorkGroupsByName("PostFX");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[1], workGroups[2]);

  _mechanics.swapchain.images[_mechanics.syncObjects.currentFrame]
      .transitionLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_GENERAL,
                        /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}
