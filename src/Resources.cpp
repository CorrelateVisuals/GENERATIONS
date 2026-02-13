#include "vulkan/vulkan.h"

#include "Mechanics.h"
#include "Resources.h"

Resources::Resources(VulkanMechanics &mechanics)
    : commands{mechanics.queues.familyIndices},
      commandInterface{commands.singularCommandBuffer, commands.pool,
                       mechanics.queues.graphics},

      depthImage{CE_DEPTH_IMAGE, mechanics.swapchain.extent,
                 CE::Image::findDepthFormat()},
      msaaImage{CE_MULTISAMPLE_IMAGE, mechanics.swapchain.extent,
                mechanics.swapchain.imageFormat},

      pushConstant{VK_SHADER_STAGE_COMPUTE_BIT, 128, 0},
      world{commands.singularCommandBuffer, commands.pool,
            mechanics.queues.graphics},

      // Descriptors
      descriptorInterface(), uniform{descriptorInterface, world._ubo},
      shaderStorage{descriptorInterface, commandInterface, world._grid.cells,
                    world._grid.pointCount},
      sampler{descriptorInterface, commandInterface,
              Lib::path("assets/Avatar.PNG")},
      storageImage{descriptorInterface, mechanics.swapchain.images} {
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "constructing Resources (start)");
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "constructing Resources");

  descriptorInterface.initialzeSets();
}

Resources::~Resources() { Log::text("{ /// }", "destructing Resources"); }

CE::ShaderAccess::CommandResources::CommandResources(
    const CE::Queues::FamilyIndices &familyIndices) {
  createPool(familyIndices);
  createBuffers(graphics);
  createBuffers(compute);
}

Resources::UniformBuffer::UniformBuffer(CE::DescriptorInterface &interface,
                                        World::UniformBufferObject &u)
    : ubo(u) {
  myIndex = interface.writeIndex;
  interface.writeIndex++;

  VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  setLayoutBinding.binding = 0;
  setLayoutBinding.descriptorType = type;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags =
      VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  interface.setLayoutBindings[myIndex] = setLayoutBinding;

  poolSize.type = type;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.poolSizes.push_back(poolSize);

  createBuffer();

  createDescriptorWrite(interface);
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

void Resources::UniformBuffer::createDescriptorWrite(
    CE::DescriptorInterface &interface) {
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
    interface.descriptorWrites[i][myIndex] = descriptorWrite;
  }
};

void Resources::UniformBuffer::update(World &world, const VkExtent2D extent) {
  ubo.light = world._ubo.light;
  ubo.gridXY = glm::vec2(static_cast<uint32_t>(world._grid.size.x),
                         static_cast<uint32_t>(world._grid.size.y));
  ubo.mvp.model = world._camera.setModel();
  ubo.mvp.view = world._camera.setView();
  ubo.mvp.projection = world._camera.setProjection(extent);

  std::memcpy(buffer.mapped, &ubo, sizeof(ubo));
}

Resources::StorageBuffer::StorageBuffer(
    CE::DescriptorInterface &descriptorInterface,
    const CE::CommandInterface &commandInterface, const auto &object,
    const size_t quantity) {
  myIndex = descriptorInterface.writeIndex;
  descriptorInterface.writeIndex += 2;

  setLayoutBinding.binding = 1;
  setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  descriptorInterface.setLayoutBindings[myIndex] = setLayoutBinding;
  setLayoutBinding.binding = 2;
  descriptorInterface.setLayoutBindings[myIndex + 1] = setLayoutBinding;

  poolSize.type = setLayoutBinding.descriptorType;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
  descriptorInterface.poolSizes.push_back(poolSize);

  create(commandInterface, object, quantity);

  createDescriptorWrite(descriptorInterface, quantity);
}

void Resources::StorageBuffer::create(
    const CE::CommandInterface &commandInterface, const auto &object,
    const size_t quantity) {
  Log::text("{ 101 }", "Shader Storage Buffers");

  // Create a staging buffer used to upload data to the gpu
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(World::Cell) * quantity;

  CE::Buffer::create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void *data;
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
                   commandInterface.commandBuffer, commandInterface.commandPool,
                   commandInterface.queue);

  CE::Buffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bufferOut);
  CE::Buffer::copy(stagingResources.buffer, bufferOut.buffer, bufferSize,
                   commandInterface.commandBuffer, commandInterface.commandPool,
                   commandInterface.queue);
}

void Resources::StorageBuffer::createDescriptorWrite(
    CE::DescriptorInterface &interface, const size_t quantity) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{.buffer = !i ? bufferIn.buffer
                                                   : bufferOut.buffer,
                                      .offset = 0,
                                      .range = sizeof(World::Cell) * quantity};

    !i ? info.currentFrame = bufferInfo : info.previousFrame = bufferInfo;

    VkWriteDescriptorSet descriptorWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstBinding = static_cast<uint32_t>(i ? 2 : 1),
        .descriptorCount = setLayoutBinding.descriptorCount,
        .descriptorType = setLayoutBinding.descriptorType,
        .pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.currentFrame)};

    interface.descriptorWrites[i][myIndex] = descriptorWrite;
    descriptorWrite.dstBinding = static_cast<uint32_t>(i ? 1 : 2);
    descriptorWrite.pBufferInfo =
        &std::get<VkDescriptorBufferInfo>(info.previousFrame);
    interface.descriptorWrites[i][myIndex + 1] = descriptorWrite;
  }
};

Resources::ImageSampler::ImageSampler(
    CE::DescriptorInterface &interface,
    const CE::CommandInterface &commandInterface,
    const std::string &texturePath)
    : textureImage(texturePath) {
  myIndex = interface.writeIndex;
  interface.writeIndex++;

  setLayoutBinding.binding = 3;
  setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  interface.setLayoutBindings[myIndex] = setLayoutBinding;

  poolSize.type = setLayoutBinding.descriptorType;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.poolSizes.push_back(poolSize);

  textureImage.loadTexture(textureImage.path, VK_FORMAT_R8G8B8A8_SRGB,
                           commandInterface.commandBuffer,
                           commandInterface.commandPool,
                           commandInterface.queue);
  textureImage.createView(VK_IMAGE_ASPECT_COLOR_BIT);
  textureImage.createSampler();

  createDescriptorWrite(interface);
}

void Resources::ImageSampler::createDescriptorWrite(
    CE::DescriptorInterface &interface) {
  VkDescriptorImageInfo imageInfo{.sampler = textureImage.sampler,
                                  .imageView = textureImage.view,
                                  .imageLayout =
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  info.currentFrame = imageInfo;

  VkWriteDescriptorSet descriptorWrite{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstBinding = setLayoutBinding.binding,
      .descriptorCount = setLayoutBinding.descriptorCount,
      .descriptorType = setLayoutBinding.descriptorType,
      .pImageInfo = &std::get<VkDescriptorImageInfo>(info.currentFrame)};

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    interface.descriptorWrites[i][myIndex] = descriptorWrite;
  }
}

Resources::StorageImage::StorageImage(
    CE::DescriptorInterface &interface,
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images) {
  myIndex = interface.writeIndex;
  interface.writeIndex++;

  setLayoutBinding.binding = 4;
  setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  setLayoutBinding.descriptorCount = 1;
  setLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  interface.setLayoutBindings[myIndex] = setLayoutBinding;

  poolSize.type = setLayoutBinding.descriptorType;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.poolSizes.push_back(poolSize);

  createDescriptorWrite(interface, images);
}

void Resources::StorageImage::createDescriptorWrite(
    CE::DescriptorInterface &interface,
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images) {
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

    interface.descriptorWrites[i][myIndex] = descriptorWrite;
  }
}
