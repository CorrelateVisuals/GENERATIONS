#include "vulkan/vulkan.h"

#include "library/Library.h"
#include "engine/Log.h"
#include "vulkan_mechanics/Mechanics.h"
#include "VulkanResources.h"

VulkanResources::VulkanResources(VulkanMechanics &mechanics, const CE::Runtime::TerrainSettings &terrain_settings)
  : commands{mechanics.queues.indices},
      command_interface{
          commands.singular_command_buffer,
          commands.pool,
          mechanics.queues.graphics_queue},

      push_constant{VK_SHADER_STAGE_COMPUTE_BIT, 8, 0},
      world{commands.singular_command_buffer,
        commands.pool,
        mechanics.queues.graphics_queue,
        terrain_settings},

      descriptor_interface(),

        depth_image{
          CE_DEPTH_IMAGE,
          mechanics.swapchain.extent,
          CE::BaseImage::find_depth_format()},
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
  Log::text("{ /// }", "constructing VulkanResources (start)");
  Log::text(Log::Style::header_guard);
  Log::text("VulkanResources initialized with provided mechanics and terrain settings.");
  Log::text("{ /// }", "constructing VulkanResources");

  descriptor_interface.initialize_sets();
}

VulkanResources::~VulkanResources() {
  Log::text("{ /// }", "destructing VulkanResources");
}

CE::ShaderAccess::CommandResources::CommandResources(
    const CE::BaseQueues::FamilyIndices &family_indices) {
  create_pool(family_indices);
  create_buffers(graphics);
  create_buffers(compute);
}

VulkanResources::UniformBuffer::UniformBuffer(CE::BaseDescriptorInterface &interface,
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

void VulkanResources::UniformBuffer::create_buffer() {
  Log::text("{ 101 }", MAX_FRAMES_IN_FLIGHT, "Uniform Buffers");
  VkDeviceSize bufferSize = sizeof(World::UniformBufferObject);

  CE::BaseBuffer::create(bufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     buffer);

  vkMapMemory(CE::BaseDevice::base_device->logical_device,
              buffer.memory,
              0,
              bufferSize,
              0,
              &buffer.mapped);
}

void VulkanResources::UniformBuffer::create_descriptor_write(CE::BaseDescriptorInterface &interface) {
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

void VulkanResources::UniformBuffer::update(World &world, const VkExtent2D extent) {
  static bool ubo_logged = false;
  ubo.light = world._ubo.light;
  ubo.grid_xy = glm::ivec2(world._grid.size.x, world._grid.size.y);
  ubo.model = world._camera.set_model();
  ubo.view = world._camera.set_view();
  ubo.projection = world._camera.set_projection(extent);

  if (!ubo_logged) {
    ubo_logged = true;
    Log::text("{ UBO }",
              "gridXY", ubo.grid_xy.x, ubo.grid_xy.y,
              "cellSize", ubo.cell_size,
              "waterThreshold", ubo.water_threshold,
              "waterRules", ubo.water_rules.x, ubo.water_rules.y,
                            ubo.water_rules.z, ubo.water_rules.w);
  }

  std::memcpy(buffer.mapped, &ubo, sizeof(ubo));
}

VulkanResources::StorageBuffer::StorageBuffer(CE::BaseDescriptorInterface &descriptor_interface,
                                        const CE::BaseCommandInterface &command_interface,
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

void VulkanResources::StorageBuffer::create(const CE::BaseCommandInterface &command_interface,
                                      const auto &object,
                                      const size_t quantity) {
  Log::text("{ 101 }", "Shader Storage Buffers");

  CE::BaseBuffer stagingResources;
  VkDeviceSize bufferSize = sizeof(World::Cell) * quantity;

  CE::BaseBuffer::create(bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingResources);

  void *data;
  vkMapMemory(CE::BaseDevice::base_device->logical_device,
              stagingResources.memory,
              0,
              bufferSize,
              0,
              &data);
  std::memcpy(data, object.data(), static_cast<size_t>(bufferSize));
  vkUnmapMemory(CE::BaseDevice::base_device->logical_device, stagingResources.memory);

  CE::BaseBuffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     buffer_in);
  CE::BaseBuffer::copy(stagingResources.buffer,
                   buffer_in.buffer,
                   bufferSize,
                   command_interface.command_buffer,
                   command_interface.command_pool,
                   command_interface.queue);

  CE::BaseBuffer::create(static_cast<VkDeviceSize>(bufferSize),
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     buffer_out);
  CE::BaseBuffer::copy(stagingResources.buffer,
                   buffer_out.buffer,
                   bufferSize,
                   command_interface.command_buffer,
                   command_interface.command_pool,
                   command_interface.queue);
}

void VulkanResources::StorageBuffer::create_descriptor_write(CE::BaseDescriptorInterface &interface,
                                                       const size_t quantity) {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{.buffer = !i ? buffer_in.buffer : buffer_out.buffer,
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

VulkanResources::ImageSampler::ImageSampler(CE::BaseDescriptorInterface &interface,
                    const CE::BaseCommandInterface &command_interface,
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

void VulkanResources::ImageSampler::create_descriptor_write(CE::BaseDescriptorInterface &interface) {
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

VulkanResources::StorageImage::StorageImage(
    CE::BaseDescriptorInterface &interface,
    std::array<CE::BaseImage, MAX_FRAMES_IN_FLIGHT> &images) {
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

void VulkanResources::StorageImage::create_descriptor_write(
    CE::BaseDescriptorInterface &interface,
    std::array<CE::BaseImage, MAX_FRAMES_IN_FLIGHT> &images) {
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
