#include "ShaderAccess.h"
#include "Pipelines.h"
#include "Resources.h"
#include "base/VulkanUtils.h"
#include "core/RuntimeConfig.h"

#include <string_view>
#include <stdexcept>

void CE::ShaderAccess::CommandResources::record_compute_command_buffer(
    Resources &resources, Pipelines &pipelines, const uint32_t frame_index) {
  VkCommandBuffer command_buffer = this->compute[frame_index];

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = nullptr;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording compute command buffer!");
  }

  vkCmdBindDescriptorSets(command_buffer,
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          pipelines.compute.layout,
                          0,
                          1,
                          &resources.descriptor_interface.sets[frame_index],
                          0,
                          nullptr);

  resources.push_constant.set_data(resources.world._time.passed_hours);

  vkCmdPushConstants(command_buffer,
                     pipelines.compute.layout,
                     resources.push_constant.shader_stage,
                     resources.push_constant.offset,
                     resources.push_constant.size,
                     resources.push_constant.data.data());

  const CE::Runtime::PipelineExecutionPlan *plan = CE::Runtime::get_pipeline_execution_plan();
  const std::vector<std::string> empty_compute{};
  const std::vector<std::string> &pre_compute =
      (plan && !plan->pre_graphics_compute.empty()) ? plan->pre_graphics_compute
                                                     : empty_compute;

  for (const std::string &pipeline_name : pre_compute) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipelines.config.get_pipeline_object_by_name(pipeline_name));

    const std::array<uint32_t, 3> &work_groups =
        pipelines.config.get_work_groups_by_name(pipeline_name);
    vkCmdDispatch(command_buffer, work_groups[0], work_groups[1], work_groups[2]);
  }

  CE::vulkan_result(vkEndCommandBuffer, command_buffer);
}

void CE::ShaderAccess::CommandResources::record_graphics_command_buffer(
    CE::Swapchain &swapchain,
    Resources &resources,
    Pipelines &pipelines,
    const uint32_t frame_index,
    const uint32_t image_index) {
  VkCommandBuffer command_buffer = this->graphics[frame_index];

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = nullptr;
  begin_info.flags = 0;
  begin_info.pInheritanceInfo = nullptr;

  CE::vulkan_result(vkBeginCommandBuffer, command_buffer, &begin_info);

  std::array<VkClearValue, 2> clear_values{
      VkClearValue{.color = {{0.46f, 0.55f, 0.62f, 1.0f}}},
      VkClearValue{.depthStencil = {1.0f, 0}}};

  VkRenderPassBeginInfo render_pass_info{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = nullptr,
      .renderPass = pipelines.render.render_pass,
      .framebuffer = swapchain.framebuffers[image_index],
      .renderArea = {.offset = {0, 0}, .extent = swapchain.extent},
      .clearValueCount = static_cast<uint32_t>(clear_values.size()),
      .pClearValues = clear_values.data()};

  vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  VkViewport viewport{.x = 0.0f,
                      .y = 0.0f,
                      .width = static_cast<float>(swapchain.extent.width),
                      .height = static_cast<float>(swapchain.extent.height),
                      .minDepth = 0.0f,
                      .maxDepth = 1.0f};
  vkCmdSetViewport(command_buffer, 0, 1, &viewport);

  VkRect2D scissor{.offset = {0, 0}, .extent = swapchain.extent};
  vkCmdSetScissor(command_buffer, 0, 1, &scissor);

  vkCmdBindDescriptorSets(command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelines.graphics.layout,
                          0,
                          1,
                          &resources.descriptor_interface.sets[frame_index],
                          0,
                          nullptr);

  const auto bind_and_draw_indexed = [&](const char *pipeline_name,
                                         VkBuffer vertex_buffer,
                                         VkBuffer index_buffer,
                                         uint32_t index_count) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.config.get_pipeline_object_by_name(pipeline_name));
    VkBuffer vertex_buffers[] = {vertex_buffer};
    VkDeviceSize indexed_offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, indexed_offsets);
    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer, index_count, 1, 0, 0, 0);
  };

  const auto draw_cells = [&](const std::string &pipeline_name) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.config.get_pipeline_object_by_name(pipeline_name));
    VkDeviceSize offsets_0[]{0, 0};

    VkBuffer current_shader_storage_buffer[] = {resources.shader_storage.buffer_in.buffer,
                                                resources.shader_storage.buffer_out.buffer};

    VkBuffer vertex_buffers_0[] = {current_shader_storage_buffer[frame_index],
                                   resources.world._cube.vertex_buffer.buffer};

    vkCmdBindVertexBuffers(command_buffer, 0, 2, vertex_buffers_0, offsets_0);
    vkCmdDraw(command_buffer,
              static_cast<uint32_t>(resources.world._cube.all_vertices.size()),
              resources.world._grid.size.x * resources.world._grid.size.y,
              0,
              0);
  };

  const auto draw_grid_indexed = [&](const std::string &pipeline_name) {
    bind_and_draw_indexed(pipeline_name.c_str(),
                          resources.world._grid.vertex_buffer.buffer,
                          resources.world._grid.index_buffer.buffer,
                          static_cast<uint32_t>(resources.world._grid.indices.size()));
  };

  const auto draw_grid_box_indexed = [&](const std::string &pipeline_name) {
    bind_and_draw_indexed(pipeline_name.c_str(),
                          resources.world._grid.box_vertex_buffer.buffer,
                          resources.world._grid.box_index_buffer.buffer,
                          static_cast<uint32_t>(resources.world._grid.box_indices.size()));
  };

  const auto draw_rectangle_indexed = [&](const std::string &pipeline_name) {
    bind_and_draw_indexed(pipeline_name.c_str(),
                          resources.world._rectangle.vertex_buffer.buffer,
                          resources.world._rectangle.index_buffer.buffer,
                          static_cast<uint32_t>(resources.world._rectangle.indices.size()));
  };

  const auto draw_cube_indexed = [&](const std::string &pipeline_name) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.config.get_pipeline_object_by_name(pipeline_name));

    VkBuffer vertex_buffers[] = {resources.world._cube.vertex_buffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    const bool has_index_data = !resources.world._cube.indices.empty() &&
                                resources.world._cube.index_buffer.buffer != VK_NULL_HANDLE;
    if (has_index_data) {
      vkCmdBindIndexBuffer(command_buffer,
                           resources.world._cube.index_buffer.buffer,
                           0,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer,
                       static_cast<uint32_t>(resources.world._cube.indices.size()),
                       1,
                       0,
                       0,
                       0);
      return;
    }

    vkCmdDraw(command_buffer,
              static_cast<uint32_t>(resources.world._cube.all_vertices.size()),
              1,
              0,
              0);
  };

  const auto draw_sky_dome = [&](const std::string &pipeline_name) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipelines.config.get_pipeline_object_by_name(pipeline_name));

    VkBuffer vertex_buffers[] = {resources.world._sky_dome.vertex_buffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    const bool has_index_data = !resources.world._sky_dome.indices.empty() &&
                                resources.world._sky_dome.index_buffer.buffer != VK_NULL_HANDLE;
    if (has_index_data) {
      vkCmdBindIndexBuffer(command_buffer,
                           resources.world._sky_dome.index_buffer.buffer,
                           0,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(command_buffer,
                       static_cast<uint32_t>(resources.world._sky_dome.indices.size()),
                       1,
                       0,
                       0,
                       0);
      return;
    }

    vkCmdDraw(command_buffer,
              static_cast<uint32_t>(resources.world._sky_dome.all_vertices.size()),
              1,
              0,
              0);
  };

  const CE::Runtime::PipelineExecutionPlan *plan = CE::Runtime::get_pipeline_execution_plan();
  const std::vector<std::string> empty_graphics{};
  const std::vector<std::string> &graphics_order =
      (plan && !plan->graphics.empty()) ? plan->graphics : empty_graphics;

  for (const std::string &pipeline_name : graphics_order) {
    const std::string *draw_op = CE::Runtime::get_graphics_draw_op(pipeline_name);
    if (!draw_op) {
      continue;
    }

    if (*draw_op == "cells_instanced" || *draw_op == "instanced:cells") {
      draw_cells(pipeline_name);
      continue;
    }

    if (*draw_op == "grid_indexed" || *draw_op == "grid_wireframe" ||
        *draw_op == "indexed:grid") {
      draw_grid_indexed(pipeline_name);
      continue;
    }

    if (*draw_op == "indexed:grid_box") {
      draw_grid_box_indexed(pipeline_name);
      continue;
    }

    if (*draw_op == "rectangle_indexed" || *draw_op == "indexed:rectangle") {
      draw_rectangle_indexed(pipeline_name);
      continue;
    }

    if (*draw_op == "indexed:cube") {
      draw_cube_indexed(pipeline_name);
      continue;
    }

    if (*draw_op == "sky_dome") {
      draw_sky_dome(pipeline_name);
      continue;
    }

    constexpr std::string_view indexed_prefix = "indexed:";
    if (draw_op->starts_with(indexed_prefix)) {
      const std::string_view target =
          std::string_view(*draw_op).substr(indexed_prefix.size());
      if (target == "grid") {
        draw_grid_indexed(pipeline_name);
      } else if (target == "grid_box") {
        draw_grid_box_indexed(pipeline_name);
      } else if (target == "cube") {
        draw_cube_indexed(pipeline_name);
      } else {
        draw_rectangle_indexed(pipeline_name);
      }
    }
  }
  vkCmdEndRenderPass(command_buffer);

  //       This is part of an image memory barrier (i.e., vkCmdPipelineBarrier
  //       with the VkImageMemoryBarrier parameter set)

  const std::vector<std::string> empty_post_compute{};
  const std::vector<std::string> &post_compute =
      (plan && !plan->post_graphics_compute.empty()) ? plan->post_graphics_compute
                                                      : empty_post_compute;

  if (!post_compute.empty()) {
    swapchain.images[image_index].transition_layout(command_buffer,
                                                    VK_FORMAT_R8G8B8A8_SRGB,
                                                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                    /* -> */ VK_IMAGE_LAYOUT_GENERAL);

    vkCmdBindDescriptorSets(command_buffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipelines.compute.layout,
                            0,
                            1,
                            &resources.descriptor_interface.sets[frame_index],
                            0,
                            nullptr);

    resources.push_constant.set_data(resources.world._time.passed_hours);
    vkCmdPushConstants(command_buffer,
                       pipelines.compute.layout,
                       resources.push_constant.shader_stage,
                       resources.push_constant.offset,
                       resources.push_constant.size,
                       resources.push_constant.data.data());

    for (const std::string &pipeline_name : post_compute) {
      vkCmdBindPipeline(command_buffer,
                        VK_PIPELINE_BIND_POINT_COMPUTE,
                        pipelines.config.get_pipeline_object_by_name(pipeline_name));
      const std::array<uint32_t, 3> &work_groups =
          pipelines.config.get_work_groups_by_name(pipeline_name);
      vkCmdDispatch(command_buffer, work_groups[0], work_groups[1], work_groups[2]);
    }

    swapchain.images[image_index].transition_layout(command_buffer,
                                                    VK_FORMAT_R8G8B8A8_SRGB,
                                                    VK_IMAGE_LAYOUT_GENERAL,
                                                    /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }

  CE::vulkan_result(vkEndCommandBuffer, command_buffer);
}
