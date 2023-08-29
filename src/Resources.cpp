#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "CapitalEngine.h"
#include "Resources.h"

World Resources::world;

Resources::Resources(VulkanMechanics& mechanics)
    : pushConstants{},
      image{},
      buffers{},
      descriptor{},
      _mechanics(mechanics) {}

Resources::~Resources() {}

void Resources::createResources(Pipelines& _pipelines) {
  Log::text("\n");
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "Resources ...");

#ifdef _WIN32
  std::string imagePath = "../assets/GenerationsCapture.PNG";
#else  // Linux-specific code
  std::string imagePath = = "assets/GenerationsCapture.PNG";
#endif

  createTextureImage(imagePath);
  createTextureImageView();
  createTextureSampler();

  createFramebuffers(_pipelines);
  createShaderStorageBuffers();
  createUniformBuffers();

  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
  createComputeCommandBuffers();
  _mechanics.createSyncObjects();
}

void Resources::createFramebuffers(Pipelines& _pipelines) {
  Log::text("{ 101 }",
            "Frame Buffers:", _mechanics.swapChain.imageViews.size());

  _mechanics.swapChain.framebuffers.resize(
      _mechanics.swapChain.imageViews.size());

  Log::text(Log::Style::charLeader, "attachments: msaa, depth, imageView*3");
  for (size_t i = 0; i < _mechanics.swapChain.imageViews.size(); i++) {
    std::vector<VkImageView> attachments{
        _pipelines.graphics.msaa.colorImageView,
        _pipelines.graphics.depth.imageView,
        _mechanics.swapChain.imageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = _pipelines.graphics.renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = _mechanics.swapChain.extent.width,
        .height = _mechanics.swapChain.extent.height,
        .layers = 1};

    _mechanics.result(vkCreateFramebuffer, _mechanics.mainDevice.logical,
                      &framebufferInfo, nullptr,
                      &_mechanics.swapChain.framebuffers[i]);
  }
}

void Resources::createCommandBuffers() {
  Log::text("{ cmd }", "Command Buffers");

  buffers.command.graphic.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = buffers.command.pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount =
          static_cast<uint32_t>(buffers.command.graphic.size())};

  _mechanics.result(vkAllocateCommandBuffers, _mechanics.mainDevice.logical,
                    &allocateInfo, buffers.command.graphic.data());
}

void Resources::createComputeCommandBuffers() {
  Log::text("{ cmd }", "Compute Command Buffers");

  buffers.command.compute.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = buffers.command.pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount =
          static_cast<uint32_t>(buffers.command.compute.size())};

  _mechanics.result(vkAllocateCommandBuffers, _mechanics.mainDevice.logical,
                    &allocateInfo, buffers.command.compute.data());
}

void Resources::createShaderStorageBuffers() {
  Log::text("{ 101 }", "Shader Storage Buffers");

  std::vector<World::Cell> cells = world.initializeCells();

  VkDeviceSize bufferSize =
      sizeof(World::Cell) * world.grid.dimensions[0] * world.grid.dimensions[1];

  // Create a staging buffer used to upload data to the gpu
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(_mechanics.mainDevice.logical, stagingBufferMemory, 0, bufferSize,
              0, &data);
  std::memcpy(data, cells.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(_mechanics.mainDevice.logical, stagingBufferMemory);

  buffers.shaderStorage.resize(MAX_FRAMES_IN_FLIGHT);
  buffers.shaderStorageMemory.resize(MAX_FRAMES_IN_FLIGHT);

  // Copy initial Cell data to all storage buffers
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(static_cast<VkDeviceSize>(bufferSize),
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffers.shaderStorage[i],
                 buffers.shaderStorageMemory[i]);
    copyBuffer(stagingBuffer, buffers.shaderStorage[i], bufferSize);
  }

  vkDestroyBuffer(_mechanics.mainDevice.logical, stagingBuffer, nullptr);
  vkFreeMemory(_mechanics.mainDevice.logical, stagingBufferMemory, nullptr);
}

void Resources::createUniformBuffers() {
  Log::text("{ 101 }", MAX_FRAMES_IN_FLIGHT, "Uniform Buffers");
  VkDeviceSize bufferSize = sizeof(World::UniformBufferObject);

  buffers.uniforms.resize(MAX_FRAMES_IN_FLIGHT);
  buffers.uniformsMemory.resize(MAX_FRAMES_IN_FLIGHT);
  buffers.uniformsMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 buffers.uniforms[i], buffers.uniformsMemory[i]);

    vkMapMemory(_mechanics.mainDevice.logical, buffers.uniformsMemory[i], 0,
                bufferSize, 0, &buffers.uniformsMapped[i]);
  }
}

void Resources::createDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT,
       .pImmutableSamplers = nullptr},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
       .pImmutableSamplers = nullptr},
      {.binding = 2,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
       .pImmutableSamplers = nullptr},
      {.binding = 3,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
       .pImmutableSamplers = nullptr},
  };

  Log::text("{ |=| }", "Descriptor Set Layout", layoutBindings.size(),
            "bindings");
  for (const VkDescriptorSetLayoutBinding& item : layoutBindings) {
    Log::text("{ ", item.binding, " }",
              Log::getDescriptorTypeString(item.descriptorType));
    Log::text(Log::Style::charLeader,
              Log::getShaderStageString(item.stageFlags));
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
      .pBindings = layoutBindings.data()};

  _mechanics.result(vkCreateDescriptorSetLayout, _mechanics.mainDevice.logical,
                    &layoutInfo, nullptr, &descriptor.setLayout);
}

void Resources::createDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSizes{
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2},
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)}};

  Log::text("{ |=| }", "Descriptor Pool");
  for (size_t i = 0; i < poolSizes.size(); i++) {
    Log::text(Log::Style::charLeader,
              Log::getDescriptorTypeString(poolSizes[i].type));
  }

  VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()};

  _mechanics.result(vkCreateDescriptorPool, _mechanics.mainDevice.logical,
                    &poolInfo, nullptr, &descriptor.pool);
}

void Resources::createImage(uint32_t width,
                            uint32_t height,
                            VkSampleCountFlagBits numSamples,
                            VkFormat format,
                            VkImageTiling tiling,
                            VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            VkImage& image,
                            VkDeviceMemory& imageMemory) {
  Log::text("{ img }", "Image", width, height);
  Log::text(Log::Style::charLeader, Log::getSampleCountString(numSamples));
  Log::text(Log::Style::charLeader, Log::getImageUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));

  VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {.width = width, .height = height, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = numSamples,
      .tiling = tiling,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = nullptr,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  _mechanics.result(vkCreateImage, _mechanics.mainDevice.logical, &imageInfo,
                    nullptr, &image);

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(_mechanics.mainDevice.logical, image,
                               &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  _mechanics.result(vkAllocateMemory, _mechanics.mainDevice.logical,
                    &allocateInfo, nullptr, &imageMemory);
  vkBindImageMemory(_mechanics.mainDevice.logical, image, imageMemory, 0);
}

void Resources::createTextureImage(std::string imagePath) {
  Log::text("{ img }", "Image Texture: ", imagePath);
  int texWidth, texHeight, texChannels;
  int rgba = 4;
  stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * rgba;

  if (!pixels) {
    throw std::runtime_error("failed to load texture image!");
  }

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuffer, stagingBufferMemory);

  void* data;
  vkMapMemory(_mechanics.mainDevice.logical, stagingBufferMemory, 0, imageSize,
              0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(_mechanics.mainDevice.logical, stagingBufferMemory);

  stbi_image_free(pixels);

  createImage(texWidth, texHeight, VK_SAMPLE_COUNT_1_BIT,
              VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image.texture,
              image.textureMemory);

  transitionImageLayout(image.texture, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stagingBuffer, image.texture,
                    static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));
  transitionImageLayout(image.texture, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(_mechanics.mainDevice.logical, stagingBuffer, nullptr);
  vkFreeMemory(_mechanics.mainDevice.logical, stagingBufferMemory, nullptr);
}

void Resources::setPushConstants() {
  pushConstants.data = {world.time.passedHours};
}
VkCommandBuffer Resources::beginSingleTimeCommands() {
  Log::text("{ 1.. }", "Begin Single Time Commands");

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = buffers.command.pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(_mechanics.mainDevice.logical, &allocInfo,
                           &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Resources::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  Log::text("{ ..1 }", "End Single Time Commands");

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(_mechanics.queues.graphics, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(_mechanics.queues.graphics);

  vkFreeCommandBuffers(_mechanics.mainDevice.logical, buffers.command.pool, 1,
                       &commandBuffer);
}

void Resources::transitionImageLayout(VkImage image,
                                      VkFormat format,
                                      VkImageLayout oldLayout,
                                      VkImageLayout newLayout) {
  Log::text("{ >>> }", "Transition Image Layout");

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void Resources::copyBufferToImage(VkBuffer buffer,
                                  VkImage image,
                                  uint32_t width,
                                  uint32_t height) {
  Log::text("{ >>> }", "Buffer To Image", width, height);

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

void Resources::createTextureImageView() {
  Log::text("{ img }", ":  Texture Image View");
  image.textureView = createImageView(image.texture, VK_FORMAT_R8G8B8A8_SRGB,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
}

void Resources::createDescriptorSets() {
  Log::text("{ |=| }", "Descriptor Sets");
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             descriptor.setLayout);
  VkDescriptorSetAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptor.pool,
      .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
      .pSetLayouts = layouts.data()};

  descriptor.sets.resize(MAX_FRAMES_IN_FLIGHT);
  _mechanics.result(vkAllocateDescriptorSets, _mechanics.mainDevice.logical,
                    &allocateInfo, descriptor.sets.data());

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo uniformBufferInfo{
        .buffer = buffers.uniforms[i],
        .offset = 0,
        .range = sizeof(World::UniformBufferObject)};

    VkDescriptorBufferInfo storageBufferInfoLastFrame{
        .buffer = buffers.shaderStorage[(i - 1) % MAX_FRAMES_IN_FLIGHT],
        .offset = 0,
        .range = sizeof(World::Cell) * world.grid.dimensions[0] *
                 world.grid.dimensions[1]};

    VkDescriptorBufferInfo storageBufferInfoCurrentFrame{
        .buffer = buffers.shaderStorage[i],
        .offset = 0,
        .range = sizeof(World::Cell) * world.grid.dimensions[0] *
                 world.grid.dimensions[1]};

    VkDescriptorImageInfo imageInfo{
        .sampler = image.textureSampler,
        .imageView = image.textureView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    std::vector<VkWriteDescriptorSet> descriptorWrites{
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = descriptor.sets[i],
         .dstBinding = 0,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &uniformBufferInfo},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = descriptor.sets[i],
         .dstBinding = 1,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .pBufferInfo = &storageBufferInfoLastFrame},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = descriptor.sets[i],
         .dstBinding = 2,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .pBufferInfo = &storageBufferInfoCurrentFrame},

        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = descriptor.sets[i],
         .dstBinding = 3,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &imageInfo},
    };

    vkUpdateDescriptorSets(_mechanics.mainDevice.logical,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void Resources::updateUniformBuffer(uint32_t currentImage) {
  World::UniformBufferObject uniformObject =
      world.updateUniforms(_mechanics.swapChain.extent);
  std::memcpy(buffers.uniformsMapped[currentImage], &uniformObject,
              sizeof(uniformObject));
}

void Resources::recordComputeCommandBuffer(VkCommandBuffer commandBuffer,
                                           Pipelines& _pipelines) {
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error(
        "failed to begin recording compute command buffer!");
  }

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    _pipelines.compute.pipeline);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          _pipelines.compute.pipelineLayout, 0, 1,
                          &descriptor.sets[_mechanics.syncObjects.currentFrame],
                          0, nullptr);

  setPushConstants();
  vkCmdPushConstants(commandBuffer, _pipelines.compute.pipelineLayout,
                     pushConstants.shaderStage, pushConstants.offset,
                     pushConstants.size, pushConstants.data.data());

  uint32_t numberOfWorkgroupsX =
      (world.grid.dimensions[0] + compute.localSizeX - 1) / compute.localSizeX;
  uint32_t numberOfWorkgroupsY =
      (world.grid.dimensions[1] + compute.localSizeY - 1) / compute.localSizeY;

  vkCmdDispatch(commandBuffer, numberOfWorkgroupsX, numberOfWorkgroupsY,
                compute.localSizeZ);

  _mechanics.result(vkEndCommandBuffer, commandBuffer);
}

void Resources::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                    uint32_t imageIndex,
                                    Pipelines& _pipelines) {
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

  _mechanics.result(vkBeginCommandBuffer, commandBuffer, &beginInfo);

  std::vector<VkClearValue> clearValues{{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
                                        {.depthStencil = {1.0f, 0}}};

  VkRenderPassBeginInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = _pipelines.graphics.renderPass,
      .framebuffer = _mechanics.swapChain.framebuffers[imageIndex],
      .renderArea = {.offset = {0, 0}, .extent = _mechanics.swapChain.extent},
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()};

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _pipelines.graphics.pipeline);

  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(_mechanics.swapChain.extent.width),
      .height = static_cast<float>(_mechanics.swapChain.extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{.offset = {0, 0}, .extent = _mechanics.swapChain.extent};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDeviceSize offsets[]{0};
  vkCmdBindVertexBuffers(
      commandBuffer, 0, 1,
      &buffers.shaderStorage[_mechanics.syncObjects.currentFrame], offsets);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          _pipelines.graphics.pipelineLayout, 0, 1,
                          &descriptor.sets[_mechanics.syncObjects.currentFrame],
                          0, nullptr);

  vkCmdDraw(commandBuffer, world.tile.vertexCount,
            world.grid.dimensions[0] * world.grid.dimensions[1], 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  _mechanics.result(vkEndCommandBuffer, commandBuffer);
}

VkImageView Resources::createImageView(VkImage image,
                                       VkFormat format,
                                       VkImageAspectFlags aspectFlags) {
  Log::text("{ img }", ":  Image View");

  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {.aspectMask = aspectFlags,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1}};

  VkImageView imageView{};
  _mechanics.result(vkCreateImageView, _mechanics.mainDevice.logical, &viewInfo,
                    nullptr, &imageView);

  return imageView;
}

void Resources::createTextureSampler() {
  Log::text("{ img }", "Texture Sampler");
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(_mechanics.mainDevice.physical, &properties);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(_mechanics.mainDevice.logical, &samplerInfo, nullptr,
                      &image.textureSampler) != VK_SUCCESS) {
    throw std::runtime_error("failed to create texture sampler!");
  }
}

void Resources::createBuffer(VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags properties,
                             VkBuffer& buffer,
                             VkDeviceMemory& bufferMemory) {
  VkBufferCreateInfo bufferInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .size = size,
                                .usage = usage,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

  Log::text("{ ... }", Log::getBufferUsageString(usage));
  Log::text(Log::Style::charLeader, Log::getMemoryPropertyString(properties));
  Log::text(Log::Style::charLeader, size, "bytes");

  _mechanics.result(vkCreateBuffer, _mechanics.mainDevice.logical, &bufferInfo,
                    nullptr, &buffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(_mechanics.mainDevice.logical, buffer,
                                &memRequirements);

  VkMemoryAllocateInfo allocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex =
          findMemoryType(memRequirements.memoryTypeBits, properties)};

  _mechanics.result(vkAllocateMemory, _mechanics.mainDevice.logical,
                    &allocateInfo, nullptr, &bufferMemory);

  vkBindBufferMemory(_mechanics.mainDevice.logical, buffer, bufferMemory, 0);
}

void Resources::copyBuffer(VkBuffer srcBuffer,
                           VkBuffer dstBuffer,
                           VkDeviceSize size) {
  Log::text("{ ... }", "copying", size, "bytes");

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

uint32_t Resources::findMemoryType(uint32_t typeFilter,
                                   VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(_mechanics.mainDevice.physical,
                                      &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("\n!ERROR! failed to find suitable memory type!");
}
