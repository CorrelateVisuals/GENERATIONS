#include "vulkan/vulkan.h"

#include "io/Library.h"
#include "core/Log.h"
#include "Mechanics.h"
#include "Resources.h"

Resources::Resources(VulkanMechanics &mechanics, const CE::Runtime::TerrainSettings &terrain_settings)
  : commands{mechanics.queues.indices},
      command_interface{
          commands.singular_command_buffer,
          commands.pool,
          mechanics.queues.graphics_queue},

      push_constant{VK_SHADER_STAGE_COMPUTE_BIT, 128, 0},
      world{commands.singular_command_buffer,
        commands.pool,
        mechanics.queues.graphics_queue,
        terrain_settings},

      // Descriptors
        descriptor_interface(),

        depth_image{
          CE_DEPTH_IMAGE,
          mechanics.swapchain.extent,
          CE::Image::find_depth_format()},
        msaa_image{CE_MULTISAMPLE_IMAGE,
                mechanics.swapchain.extent,
            mechanics.swapchain.image_format},

        uniform{descriptor_interface, world._ubo}, shader_storage{descriptor_interface,
                                    command_interface,
                                    world._grid.cells,
                                      world._grid.point_count},
        sampler{descriptor_interface, command_interface, Lib::path("assets/Avatar.PNG")},
        storage_image{descriptor_interface, mechanics.swapchain.images} {
  Log::text(Log::Style::header_guard);
  Log::text("{ /// }", "constructing Resources (start)");
  Log::text(Log::Style::header_guard);
  Log::text("{ /// }", "constructing Resources");

  descriptor_interface.initialize_sets();
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
  set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT |
                                  VK_SHADER_STAGE_VERTEX_BIT |
                                  VK_SHADER_STAGE_FRAGMENT_BIT;
  interface.set_layout_bindings[my_index] = set_layout_binding;

  pool_size.type = type;
  pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;
  interface.pool_sizes.push_back(pool_size);

  create_buffer();

  create_descriptor_write(interface);
}

void Resources::UniformBuffer::create_buffer() {
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

VkDescriptorBufferInfo Resources::UniformBuffer::create_buffer_info() const {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer.buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(World::UniformBufferObject);
  return bufferInfo;
}

VkWriteDescriptorSet Resources::UniformBuffer::create_write_descriptor_set() const {
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
  return descriptorWrite;
}

void Resources::UniformBuffer::create_descriptor_write(CE::DescriptorInterface &interface) {
  info.current_frame = create_buffer_info();
  VkWriteDescriptorSet descriptorWrite = create_write_descriptor_set();
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    interface.descriptor_writes[i][my_index] = descriptorWrite;
  }
};

void Resources::UniformBuffer::update(World &world, const VkExtent2D extent) {
  ubo.light = world._ubo.light;
  ubo.grid_xy = glm::vec2(static_cast<uint32_t>(world._grid.size.x),
                          static_cast<uint32_t>(world._grid.size.y));
  ubo.mvp.model = world._camera.set_model();
  ubo.mvp.view = world._camera.set_view();
  ubo.mvp.projection = world._camera.set_projection(extent);

  std::memcpy(buffer.mapped, &ubo, sizeof(ubo));
}

Resources::StorageBuffer::StorageBuffer(CE::DescriptorInterface &descriptor_interface,
                                        const CE::CommandInterface &command_interface,
                                        const auto &object,
                                        const size_t quantity) {
  my_index = descriptor_interface.write_index;
  descriptor_interface.write_index += 2;

  set_layout_binding.binding = 1;
  set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  set_layout_binding.descriptorCount = 1;
  set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  descriptor_interface.set_layout_bindings[my_index] = set_layout_binding;
  set_layout_binding.binding = 2;
  descriptor_interface.set_layout_bindings[my_index + 1] = set_layout_binding;

  pool_size.type = set_layout_binding.descriptorType;
  pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT * 2;
  descriptor_interface.pool_sizes.push_back(pool_size);

  create(command_interface, object, quantity);

  create_descriptor_write(descriptor_interface, quantity);
}

void Resources::StorageBuffer::map_and_copy_data(CE::Buffer &buffer, const auto &object, VkDeviceSize size) {
  void *data;
  vkMapMemory(CE::Device::base_device->logical_device, buffer.memory, 0, size, 0, &data);
  std::memcpy(data, object.data(), static_cast<size_t>(size));
  vkUnmapMemory(CE::Device::base_device->logical_device, buffer.memory);
}

CE::Buffer Resources::StorageBuffer::create_staging_buffer(const auto &object, const size_t quantity, VkDeviceSize &out_buffer_size) {
  CE::Buffer stagingResources;
  VkDeviceSize bufferSize = sizeof(World::Cell) * quantity;
  out_buffer_size = bufferSize;
  CE::Buffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);
  map_and_copy_data(stagingResources, object, bufferSize);
  return stagingResources;
}

void Resources::StorageBuffer::create_device_buffer(const CE::CommandInterface &command_interface,
                                                     const CE::Buffer &staging,
                                                     VkDeviceSize size,
                                                     CE::Buffer &target) {
  CE::Buffer::create(static_cast<VkDeviceSize>(size),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     target);
  CE::Buffer::copy(staging.buffer, target.buffer, size,
                   command_interface.command_buffer,
                   command_interface.command_pool,
                   command_interface.queue);
}

void Resources::StorageBuffer::create(const CE::CommandInterface &command_interface,
                                      const auto &object,
                                      const size_t quantity) {
  Log::text("{ 101 }", "Shader Storage Buffers");
  VkDeviceSize bufferSize;
  CE::Buffer stagingResources = create_staging_buffer(object, quantity, bufferSize);
  create_device_buffer(command_interface, stagingResources, bufferSize, buffer_in);
  create_device_buffer(command_interface, stagingResources, bufferSize, buffer_out);
}

VkDescriptorBufferInfo Resources::StorageBuffer::create_buffer_info(uint32_t frame_index, const size_t quantity) const {
  VkDescriptorBufferInfo bufferInfo{
      .buffer = !frame_index ? buffer_in.buffer : buffer_out.buffer,
      .offset = 0,
      .range = sizeof(World::Cell) * quantity};
  return bufferInfo;
}

VkWriteDescriptorSet Resources::StorageBuffer::create_write_set(uint32_t frame_index) const {
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.dstSet = VK_NULL_HANDLE;
  descriptorWrite.dstBinding = static_cast<uint32_t>(frame_index ? 2 : 1);
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = set_layout_binding.descriptorCount;
  descriptorWrite.descriptorType = set_layout_binding.descriptorType;
  descriptorWrite.pImageInfo = nullptr;
  descriptorWrite.pTexelBufferView = nullptr;
  return descriptorWrite;
}

void Resources::StorageBuffer::update_descriptor_writes(CE::DescriptorInterface &interface, uint32_t frame_index, const VkWriteDescriptorSet &desc_write) {
  VkWriteDescriptorSet descriptorWrite = desc_write;
  descriptorWrite.pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.current_frame);
  interface.descriptor_writes[frame_index][my_index] = descriptorWrite;
  descriptorWrite.dstBinding = static_cast<uint32_t>(frame_index ? 1 : 2);
  descriptorWrite.pBufferInfo = &std::get<VkDescriptorBufferInfo>(info.previous_frame);
  interface.descriptor_writes[frame_index][my_index + 1] = descriptorWrite;
}

void Resources::StorageBuffer::create_descriptor_write(CE::DescriptorInterface &interface,
                                                       const size_t quantity) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = create_buffer_info(i, quantity);
    !i ? info.current_frame = bufferInfo : info.previous_frame = bufferInfo;
    VkWriteDescriptorSet descriptorWrite = create_write_set(i);
    update_descriptor_writes(interface, i, descriptorWrite);
  }
};

Resources::ImageSampler::ImageSampler(CE::DescriptorInterface &interface,
                    const CE::CommandInterface &command_interface,
                    const std::string &texture_path)
  : texture_image(texture_path) {
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

  texture_image.load_texture(texture_path,
                             VK_FORMAT_R8G8B8A8_SRGB,
                             command_interface.command_buffer,
                             command_interface.command_pool,
                             command_interface.queue);
  texture_image.create_view(VK_IMAGE_ASPECT_COLOR_BIT);
  texture_image.create_sampler();

  create_descriptor_write(interface);
}

void Resources::ImageSampler::create_descriptor_write(CE::DescriptorInterface &interface) {
  VkDescriptorImageInfo imageInfo{.sampler = texture_image.sampler,
                                  .imageView = texture_image.view,
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

  create_descriptor_write(interface, images);
}

VkDescriptorImageInfo Resources::StorageImage::create_image_info(uint32_t frame_index, std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images) const {
  VkDescriptorImageInfo imageInfo{
      .sampler = VK_NULL_HANDLE,
      .imageView = images[frame_index].view,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL};
  return imageInfo;
}

VkWriteDescriptorSet Resources::StorageImage::create_write_set(uint32_t frame_index) const {
  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.pNext = nullptr;
  descriptorWrite.dstSet = VK_NULL_HANDLE;
  descriptorWrite.dstBinding = set_layout_binding.binding;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorCount = set_layout_binding.descriptorCount;
  descriptorWrite.descriptorType = set_layout_binding.descriptorType;
  descriptorWrite.pImageInfo = &std::get<VkDescriptorImageInfo>(!frame_index ? info.current_frame : info.previous_frame);
  descriptorWrite.pBufferInfo = nullptr;
  descriptorWrite.pTexelBufferView = nullptr;
  return descriptorWrite;
}

void Resources::StorageImage::create_descriptor_write(
    CE::DescriptorInterface &interface,
    std::array<CE::Image, MAX_FRAMES_IN_FLIGHT> &images) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorImageInfo imageInfo = create_image_info(i, images);
    !i ? info.current_frame = imageInfo : info.previous_frame = imageInfo;
    VkWriteDescriptorSet descriptorWrite = create_write_set(i);
    interface.descriptor_writes[i][my_index] = descriptorWrite;
  }
}
