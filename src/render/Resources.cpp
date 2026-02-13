#include "vulkan/vulkan.h"

#include "io/Library.h"
#include "core/Log.h"
#include "Mechanics.h"
#include "Resources.h"

Resources::Resources(VulkanMechanics &mechanics)
    : commands{mechanics.queues.family_indices},
      commandInterface{
          commands.singular_command_buffer,
          commands.pool,
          mechanics.queues.graphics_queue},

      push_constant{VK_SHADER_STAGE_COMPUTE_BIT, 128, 0},
      world{commands.singular_command_buffer,
        commands.pool,
        mechanics.queues.graphics_queue},

      // Descriptors
      descriptorInterface(),

      depthImage{
          CE_DEPTH_IMAGE,
          mechanics.swapchain.extent,
          CE::Image::find_depth_format()},
      msaaImage{CE_MULTISAMPLE_IMAGE,
                mechanics.swapchain.extent,
            mechanics.swapchain.image_format},

      uniform{descriptorInterface, world._ubo}, shaderStorage{descriptorInterface,
                                                              commandInterface,
                                                              world._grid.cells,
                                                              world._grid.pointCount},
      sampler{descriptorInterface, commandInterface, Lib::path("assets/Avatar.PNG")},
      storageImage{descriptorInterface, mechanics.swapchain.images} {
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "constructing Resources (start)");
  Log::text(Log::Style::headerGuard);
  Log::text("{ /// }", "constructing Resources");

  descriptorInterface.initialize_sets();
}

Resources::~Resources() {
  Log::text("{ /// }", "destructing Resources");
}

CE::ShaderAccess::CommandResources::CommandResources(
    const CE::Queues::FamilyIndices &family_indices) {
  create_pool(family_indices);
  create_buffers(graphics);
  create_buffers(compute);
}

Resources::UniformBuffer::UniformBuffer(CE::DescriptorInterface &interface,
                                        World::UniformBufferObject &u)
    : ubo(u) {
  my_index = interface.write_index;
  interface.write_index++;

  VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  set_layout_binding.binding = 0;
  set_layout_binding.descriptorType = type;
  set_layout_binding.descriptorCount = 1;
  set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  interface.set_layout_bindings[my_index] = set_layout_binding;

  pool_size.type = type;
  pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.pool_sizes.push_back(pool_size);

  createBuffer();

  createDescriptorWrite(interface);
}

void Resources::UniformBuffer::createBuffer() {
  Log::text("{ 101 }", MAX_FRAMES_IN_FLIGHT, "Uniform Buffers");
  VkDeviceSize bufferSize = sizeof(World::UniformBufferObject);

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     buffer);

  vkMapMemory(CE::Device::base_device->logical_device,
              buffer.memory,
              0,
              bufferSize,
              0,
              &buffer.mapped);
}

void Resources::UniformBuffer::createDescriptorWrite(CE::DescriptorInterface &interface) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer.buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(World::UniformBufferObject);
  info.current_frame = bufferInfo;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.dstSet = VK_NULL_HANDLE;
  descriptorWrite.dstBinding = set_layout_binding.binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = set_layout_binding.descriptorCount;
  descriptorWrite.descriptorType = set_layout_binding.descriptorType;
  descriptorWrite.pImageInfo = nullptr;
  descriptorWrite.pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.current_frame);
  descriptorWrite.pTexelBufferView = nullptr;

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    interface.descriptor_writes[i][my_index] = descriptorWrite;
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

Resources::StorageBuffer::StorageBuffer(CE::DescriptorInterface &descriptorInterface,
                                        const CE::CommandInterface &commandInterface,
                                        const auto &object,
                                        const size_t quantity) {
  my_index = descriptorInterface.write_index;
  descriptorInterface.write_index += 2;

  set_layout_binding.binding = 1;
  set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  set_layout_binding.descriptorCount = 1;
  set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  descriptorInterface.set_layout_bindings[my_index] = set_layout_binding;
  set_layout_binding.binding = 2;
  descriptorInterface.set_layout_bindings[my_index + 1] = set_layout_binding;

  pool_size.type = set_layout_binding.descriptorType;
  pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
  descriptorInterface.pool_sizes.push_back(pool_size);

  create(commandInterface, object, quantity);

  createDescriptorWrite(descriptorInterface, quantity);
}

void Resources::StorageBuffer::create(const CE::CommandInterface &commandInterface,
                                      const auto &object,
                                      const size_t quantity) {
  Log::text("{ 101 }", "Shader Storage Buffers");

  // Create a staging buffer used to upload data to the gpu
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(World::Cell) * quantity;

  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void *data;
  vkMapMemory(CE::Device::base_device->logical_device,
              stagingResources.memory,
              0,
              bufferSize,
              0,
              &data);
  std::memcpy(data, object.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(CE::Device::base_device->logical_device, stagingResources.memory);

  CE::Buffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     bufferIn);
  CE::Buffer::copy(stagingResources.buffer,
                   bufferIn.buffer,
                   bufferSize,
                   commandInterface.command_buffer,
                   commandInterface.command_pool,
                   commandInterface.queue);

  CE::Buffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     bufferOut);
  CE::Buffer::copy(stagingResources.buffer,
                   bufferOut.buffer,
                   bufferSize,
                   commandInterface.command_buffer,
                   commandInterface.command_pool,
                   commandInterface.queue);
}

void Resources::StorageBuffer::createDescriptorWrite(CE::DescriptorInterface &interface,
                                                     const size_t quantity) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{.buffer = !i ? bufferIn.buffer : bufferOut.buffer,
                                      .offset = 0,
                                      .range = sizeof(World::Cell) * quantity};

    !i ? info.current_frame = bufferInfo : info.previous_frame = bufferInfo;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.dstBinding = static_cast<uint32_t>(i ? 2 : 1);
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = set_layout_binding.descriptorCount;
    descriptorWrite.descriptorType = set_layout_binding.descriptorType;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.current_frame);
    descriptorWrite.pTexelBufferView = nullptr;

    interface.descriptor_writes[i][my_index] = descriptorWrite;
    descriptorWrite.dstBinding = static_cast<uint32_t>(i ? 1 : 2);
    descriptorWrite.pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.previous_frame);
    interface.descriptor_writes[i][my_index + 1] = descriptorWrite;
  }
};

Resources::ImageSampler::ImageSampler(CE::DescriptorInterface &interface,
                                      const CE::CommandInterface &commandInterface,
                                      const std::string &texturePath)
    : textureImage(texturePath) {
  my_index = interface.write_index;
  interface.write_index++;

  set_layout_binding.binding = 3;
  set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  set_layout_binding.descriptorCount = 1;
  set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  interface.set_layout_bindings[my_index] = set_layout_binding;

  pool_size.type = set_layout_binding.descriptorType;
  pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.pool_sizes.push_back(pool_size);

  textureImage.load_texture(textureImage.path,
                            VK_FORMAT_R8G8B8A8_SRGB,
                            commandInterface.command_buffer,
                            commandInterface.command_pool,
                            commandInterface.queue);
  textureImage.create_view(VK_IMAGE_ASPECT_COLOR_BIT);
  textureImage.create_sampler();

  createDescriptorWrite(interface);
}

void Resources::ImageSampler::createDescriptorWrite(CE::DescriptorInterface &interface) {
  VkDescriptorImageInfo imageInfo{.sampler = textureImage.sampler,
                                  .imageView = textureImage.view,
                                  .imageLayout =
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  info.current_frame = imageInfo;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.dstSet = VK_NULL_HANDLE;
  descriptorWrite.dstBinding = set_layout_binding.binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = set_layout_binding.descriptorCount;
  descriptorWrite.descriptorType = set_layout_binding.descriptorType;
  descriptorWrite.pImageInfo = &std::get<VkDescriptorImageInfo>(info.current_frame);
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pTexelBufferView = nullptr;

  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    interface.descriptor_writes[i][my_index] = descriptorWrite;
  }
}

Resources::StorageImage::StorageImage(
    CE::DescriptorInterface &interface,
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images) {
  my_index = interface.write_index;
  interface.write_index++;

  set_layout_binding.binding = 4;
  set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  set_layout_binding.descriptorCount = 1;
  set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  interface.set_layout_bindings[my_index] = set_layout_binding;

  pool_size.type = set_layout_binding.descriptorType;
  pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.pool_sizes.push_back(pool_size);

  createDescriptorWrite(interface, images);
}

void Resources::StorageImage::createDescriptorWrite(
    CE::DescriptorInterface &interface,
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorImageInfo imageInfo{.sampler = VK_NULL_HANDLE,
                                    .imageView = images[i].view,
                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL};

    !i ? info.current_frame = imageInfo : info.previous_frame = imageInfo;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.pNext = nullptr;
    descriptorWrite.dstSet = VK_NULL_HANDLE;
    descriptorWrite.dstBinding = set_layout_binding.binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = set_layout_binding.descriptorCount;
    descriptorWrite.descriptorType = set_layout_binding.descriptorType;
    descriptorWrite.pImageInfo =
        &std::get<VkDescriptorImageInfo>(!i ? info.current_frame : info.previous_frame);
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    interface.descriptor_writes[i][my_index] = descriptorWrite;
  }
}
