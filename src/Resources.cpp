#include "vulkan/vulkan.h"

#include "CapitalEngine.h"
#include "Resources.h"

namespace {}

Resources::Resources(VulkanMechanics& mechanics, Pipelines& pipelines)
    : commands{mechanics.queues.familyIndices},
      commandInterface{commands.singularCommandBuffer, commands.pool,
                       mechanics.queues.graphics},

      world{commands.singularCommandBuffer, commands.pool,
            mechanics.queues.graphics},
      pushConstant{VK_SHADER_STAGE_COMPUTE_BIT, 128, 0},

      uniform{world._ubo},
      shaderStorage{commandInterface, world._grid.cells, world._grid.pointCount},
      sampler{commandInterface, Lib::path("assets/Avatar.PNG")},
      storageImage{mechanics.swapchain.images},

      depthImage{CE_DEPTH_IMAGE, mechanics.swapchain.extent,
                 CE::Image::findDepthFormat()},
      msaaImage{CE_MULTISAMPLE_IMAGE, mechanics.swapchain.extent,
                mechanics.swapchain.imageFormat}

{
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "constructing Resources");

  CE::Descriptor::createSetLayout(CE::Descriptor::setLayoutBindings);
  CE::Descriptor::createPool();
  CE::Descriptor::allocateSets();
  CE::Descriptor::updateSets(CE::Descriptor::sets,
                             CE::Descriptor::descriptorWrites);
}

Resources::~Resources() {
  Log::text("{ /// }", "destructing Resources");
}

Resources::CommandResources::CommandResources(
    const CE::Queues::FamilyIndices& familyIndices) {
  createPool(familyIndices);
  createBuffers(graphics);
  createBuffers(compute);
}

Resources::UniformBuffer::UniformBuffer(World::UniformBufferObject& u)
    : _ubo(u) {
  myIndex = writeIndex;
  writeIndex++;

  VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  setLayoutBinding.binding = 0;
  setLayoutBinding.descriptorType = type;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  setLayoutBindings[myIndex] = setLayoutBinding;

  poolSize.type = type;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  poolSizes.push_back(poolSize);

  createBuffer();

  createDescriptorWrite();
}

void Resources::UniformBuffer::createBuffer() {
  Log::text("{ 101 }", MAX_FRAMES_IN_FLIGHT, "Uniform Buffers");
  VkDeviceSize bufferSize = sizeof(World::UniformBufferObject);

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     buffer);

  vkMapMemory(CE::Device::baseDevice->logical, buffer.memory, 0, bufferSize, 0,
              &buffer.mapped);
}

void Resources::UniformBuffer::createDescriptorWrite() {
  VkDescriptorBufferInfo bufferInfo{
      .buffer = buffer.buffer, .range = sizeof(World::UniformBufferObject)};
  info.currentFrame = bufferInfo;

  VkWriteDescriptorSet descriptorWrite{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstBinding = setLayoutBinding.binding,
      .descriptorCount = setLayoutBinding.descriptorCount,
      .descriptorType = setLayoutBinding.descriptorType,
      .pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.currentFrame)};

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    descriptorWrites[i][myIndex] = descriptorWrite;
  }
};

void Resources::UniformBuffer::update(World& world, const VkExtent2D extent) {
  _ubo.light = world._ubo.light;
  _ubo.gridXY = glm::vec2(static_cast<uint32_t>(world._grid.size.x),
                         static_cast<uint32_t>(world._grid.size.y));
  _ubo.mvp.model = world._camera.setModel();
  _ubo.mvp.view = world._camera.setView();
  _ubo.mvp.projection = world._camera.setProjection(extent);

  std::memcpy(buffer.mapped, &_ubo, sizeof(_ubo));
}

Resources::StorageBuffer::StorageBuffer(const CE::CommandInterface& commandData,
                                        const auto& object,
                                        const size_t quantity) {
  myIndex = writeIndex;
  writeIndex += 2;

  setLayoutBinding.binding = 1;
  setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  setLayoutBindings[myIndex] = setLayoutBinding;
  setLayoutBinding.binding = 2;
  setLayoutBindings[myIndex + 1] = setLayoutBinding;

  poolSize.type = setLayoutBinding.descriptorType;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
  poolSizes.push_back(poolSize);

  create(commandData, object, quantity);

  createDescriptorWrite(quantity);
}

void Resources::StorageBuffer::create(const CE::CommandInterface& commandData,
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
                   commandData.commandBuffer, commandData.commandPool,
                   commandData.queue);

  CE::Buffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferOut);
  CE::Buffer::copy(stagingResources.buffer, bufferOut.buffer, bufferSize,
                   commandData.commandBuffer, commandData.commandPool,
                   commandData.queue);
}

void Resources::StorageBuffer::createDescriptorWrite(const size_t quantity) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{
        .buffer = !i ? bufferIn.buffer : bufferOut.buffer,
        .offset = 0,
        .range = sizeof(World::Cell) * quantity};

    !i ? info.currentFrame = bufferInfo : info.previousFrame = bufferInfo;

    VkWriteDescriptorSet descriptorWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = static_cast<uint32_t>(i ? 2 : 1),
        .descriptorCount = setLayoutBinding.descriptorCount,
        .descriptorType = setLayoutBinding.descriptorType,
        .pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.currentFrame)};

    descriptorWrites[i][myIndex] = descriptorWrite;
    descriptorWrite.dstBinding = static_cast<uint32_t>(i ? 1 : 2);
    descriptorWrite.pBufferInfo =
        &std::get<VkDescriptorBufferInfo>(info.previousFrame);
    descriptorWrites[i][myIndex + 1] = descriptorWrite;
  }
};

Resources::ImageSampler::ImageSampler(const CE::CommandInterface& commandData,
                                      const std::string& texturePath)
    : textureImage(texturePath) {
  myIndex = writeIndex;
  writeIndex++;

  setLayoutBinding.binding = 3;
  setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  setLayoutBindings[myIndex] = setLayoutBinding;

  poolSize.type = setLayoutBinding.descriptorType;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  poolSizes.push_back(poolSize);

  textureImage.loadTexture(textureImage.path, VK_FORMAT_R8G8B8A8_SRGB,
                           commandData.commandBuffer, commandData.commandPool,
                           commandData.queue);
  textureImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);
  textureImage.createSampler();

  createDescriptorWrite();
}

void Resources::ImageSampler::createDescriptorWrite() {
  VkDescriptorImageInfo imageInfo{
      .sampler = textureImage.sampler,
      .imageView = textureImage.view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  info.currentFrame = imageInfo;

  VkWriteDescriptorSet descriptorWrite{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstBinding = setLayoutBinding.binding,
      .descriptorCount = setLayoutBinding.descriptorCount,
      .descriptorType = setLayoutBinding.descriptorType,
      .pImageInfo = &std::get<VkDescriptorImageInfo>(info.currentFrame)};

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    descriptorWrites[i][myIndex] = descriptorWrite;
  }
}

Resources::StorageImage::StorageImage(
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images) {
  myIndex = writeIndex;
  writeIndex++;

  setLayoutBinding.binding = 4;
  setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  setLayoutBindings[myIndex] = setLayoutBinding;

  poolSize.type = setLayoutBinding.descriptorType;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  poolSizes.push_back(poolSize);

  createDescriptorWrite(images);
}

void Resources::StorageImage::createDescriptorWrite(
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT>& images) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                    .imageView = images[i].view,
                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

    !i ? info.currentFrame = imageInfo : info.previousFrame = imageInfo;

    VkWriteDescriptorSet descriptorWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = setLayoutBinding.binding,
        .descriptorCount = setLayoutBinding.descriptorCount,
        .descriptorType = setLayoutBinding.descriptorType,
        .pImageInfo = &std::get<VkDescriptorImageInfo>(
            !i ? info.currentFrame : info.previousFrame)};

    descriptorWrites[i][myIndex] = descriptorWrite;
  }
}

void Resources::CommandResources::recordComputeCommandBuffer(
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

  resources.pushConstant.setData(resources.world._time.passedHours);

  vkCmdPushConstants(commandBuffer, pipelines.compute.layout,
                     resources.pushConstant.shaderStage,
                     resources.pushConstant.offset, resources.pushConstant.size,
                     resources.pushConstant.data.data());

  const std::array<uint32_t, 3>& workGroups =
      pipelines.config.getWorkGroupsByName("Engine");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[0], workGroups[2]);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}

void Resources::CommandResources::recordGraphicsCommandBuffer(
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
                               resources.world._cube.vertexBuffer.buffer};

  vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers0, offsets0);
  vkCmdDraw(commandBuffer,
            static_cast<uint32_t>(resources.world._cube.allVertices.size()),
            resources.world._grid.size.x * resources.world._grid.size.y, 0, 0);

  // Landscape
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Landscape"));
  VkBuffer vertexBuffers1[] = {resources.world._grid.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers1, offsets);
  vkCmdBindIndexBuffer(commandBuffer, resources.world._grid.indexBuffer.buffer,
                       0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._grid.indices.size()),
                   1, 0, 0, 0);

  //   Landscape Wireframe
  vkCmdBindPipeline(
      commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelines.config.getPipelineObjectByName("LandscapeWireFrame"));
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._grid.indices.size()),
                   1, 0, 0, 0);

  // Pipeline 3
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Water"));
  VkBuffer vertexBuffers[] = {resources.world._rectangle.vertexBuffer.buffer};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, resources.world._rectangle.indexBuffer.buffer,
                       0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._rectangle.indices.size()),
                   1, 0, 0, 0);

  // Pipeline 4
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.config.getPipelineObjectByName("Texture"));
  vkCmdDrawIndexed(commandBuffer,
                   static_cast<uint32_t>(resources.world._rectangle.indices.size()),
                   1, 0, 0, 0);
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

  resources.pushConstant.setData(resources.world._time.passedHours);
  vkCmdPushConstants(commandBuffer, pipelines.compute.layout,
                     resources.pushConstant.shaderStage,
                     resources.pushConstant.offset, resources.pushConstant.size,
                     resources.pushConstant.data.data());

  const std::array<uint32_t, 3>& workGroups =
      pipelines.config.getWorkGroupsByName("PostFX");
  vkCmdDispatch(commandBuffer, workGroups[0], workGroups[1], workGroups[2]);

  swapchain.images[imageIndex].transitionLayout(
      commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_GENERAL,
      /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  CE::VULKAN_RESULT(vkEndCommandBuffer, commandBuffer);
}
