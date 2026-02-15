#include "ShaderAccess.h"
#include "Pipelines.h"
#include "Resources.h"
#include "gui.h"
#include "base/VulkanUtils.h"
#include "core/RuntimeConfig.h"

#include <array>
#include <algorithm>
#include <string_view>
#include <stdexcept>
#include <vector>

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

  resources.push_constant.set_data(resources.world._time.passed_hours,
                                   resources.world._time.get_day_fraction());

  vkCmdPushConstants(command_buffer,
                     pipelines.compute.layout,
                     resources.push_constant.shader_stage,
                     resources.push_constant.offset,
                     resources.push_constant.size,
                     resources.push_constant.data.data());

  const CE::Runtime::RenderGraph *render_graph = CE::Runtime::get_render_graph();
  const CE::Runtime::PipelineExecutionPlan *plan = CE::Runtime::get_pipeline_execution_plan();
  std::vector<std::string> pre_compute;
  if (render_graph) {
    for (const CE::Runtime::RenderNode &node : render_graph->nodes) {
      if (node.stage == CE::Runtime::RenderStage::PreCompute) {
        pre_compute.push_back(node.pipeline);
      }
    }
  } else if (plan) {
    pre_compute = plan->pre_graphics_compute;
  }

  const bool run_startup_seed = resources.startup_seed_pending;
  if (run_startup_seed) {
    pre_compute.insert(pre_compute.begin(), "SeedCells");
  }

  const auto insert_compute_barrier = [&](VkCommandBuffer buffer) {
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(buffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         1,
                         &barrier,
                         0,
                         nullptr,
                         0,
                         nullptr);
  };

  for (std::size_t i = 0; i < pre_compute.size(); ++i) {
    const std::string &pipeline_name = pre_compute[i];
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipelines.config.get_pipeline_object_by_name(pipeline_name));

    const std::array<uint32_t, 3> &work_groups =
        pipelines.config.get_work_groups_by_name(pipeline_name);
    vkCmdDispatch(command_buffer, work_groups[0], work_groups[1], work_groups[2]);
    if (i + 1 < pre_compute.size()) {
      insert_compute_barrier(command_buffer);
    }
  }

  if (run_startup_seed) {
    resources.startup_seed_pending = false;
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
  const bool stage_strip_potentially_enabled = CE::RenderGUI::is_stage_strip_enabled();
  bool stage_strip_enabled = false;
  CE::RenderGUI::StageStripConfig stage_strip{};

  if (stage_strip_potentially_enabled) {
    stage_strip = CE::RenderGUI::get_stage_strip_config(swapchain.extent);
    stage_strip_enabled =
        stage_strip.enabled && swapchain.extent.height > (stage_strip.strip_height_px + 1);
  }

  if (stage_strip_enabled) {
    VkRect2D scene_scissor{
        .offset = {0, static_cast<int32_t>(stage_strip.strip_height_px)},
        .extent = {.width = swapchain.extent.width,
                   .height = swapchain.extent.height - stage_strip.strip_height_px}};
    vkCmdSetScissor(command_buffer, 0, 1, &scene_scissor);
  } else {
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  }

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

    VkBuffer current_shader_storage_buffer[] = {resources.shader_storage.buffer_out.buffer,
                          resources.shader_storage.buffer_in.buffer};

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

  const auto draw_pipeline_from_draw_op_id = [&](const std::string &pipeline_name,
                                                 const CE::Runtime::DrawOpId draw_op_id) {
    if (draw_op_id == CE::Runtime::DrawOpId::InstancedCells) {
      draw_cells(pipeline_name);
      return;
    }

    if (draw_op_id == CE::Runtime::DrawOpId::IndexedGrid) {
      draw_grid_indexed(pipeline_name);
      return;
    }

    if (draw_op_id == CE::Runtime::DrawOpId::IndexedGridBox) {
      draw_grid_box_indexed(pipeline_name);
      return;
    }

    if (draw_op_id == CE::Runtime::DrawOpId::IndexedRectangle) {
      draw_rectangle_indexed(pipeline_name);
      return;
    }

    if (draw_op_id == CE::Runtime::DrawOpId::IndexedCube) {
      draw_cube_indexed(pipeline_name);
      return;
    }

    if (draw_op_id == CE::Runtime::DrawOpId::SkyDome) {
      draw_sky_dome(pipeline_name);
      return;
    }
  };

  const auto draw_pipeline_from_draw_op_string = [&](const std::string &pipeline_name,
                                                      const std::string &draw_op) {
    const CE::Runtime::DrawOpId draw_op_id = CE::Runtime::draw_op_from_string(draw_op);
    if (draw_op_id != CE::Runtime::DrawOpId::Unknown) {
      draw_pipeline_from_draw_op_id(pipeline_name, draw_op_id);
      return;
    }

    constexpr std::string_view indexed_prefix = "indexed:";
    if (draw_op.starts_with(indexed_prefix)) {
      const std::string_view target =
          std::string_view(draw_op).substr(indexed_prefix.size());
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
  };

  const auto draw_pipeline_by_name = [&](const std::string &pipeline_name) {
    const CE::Runtime::DrawOpId draw_op_id = CE::Runtime::get_graphics_draw_op_id(pipeline_name);
    if (draw_op_id != CE::Runtime::DrawOpId::Unknown) {
      draw_pipeline_from_draw_op_id(pipeline_name, draw_op_id);
      return;
    }

    const std::string *draw_op = CE::Runtime::get_graphics_draw_op(pipeline_name);
    if (draw_op) {
      draw_pipeline_from_draw_op_string(pipeline_name, *draw_op);
    }
  };

  const CE::Runtime::PipelineExecutionPlan *legacy_plan =
      CE::Runtime::get_pipeline_execution_plan();
  const CE::Runtime::RenderGraph *graphics_render_graph = CE::Runtime::get_render_graph();
  if (graphics_render_graph) {
    for (const CE::Runtime::RenderNode &node : graphics_render_graph->nodes) {
      if (node.stage != CE::Runtime::RenderStage::Graphics) {
        continue;
      }
      if (node.draw_op != CE::Runtime::DrawOpId::Unknown) {
        draw_pipeline_from_draw_op_id(node.pipeline, node.draw_op);
      } else {
        draw_pipeline_by_name(node.pipeline);
      }
    }
  } else {
    if (legacy_plan) {
      for (const std::string &pipeline_name : legacy_plan->graphics) {
        draw_pipeline_by_name(pipeline_name);
      }
    }
  }

  if (stage_strip_enabled) {
    const std::vector<CE::RenderGUI::StageStripTile> &strip_tiles =
        CE::RenderGUI::get_stage_strip_tiles();
    if (!strip_tiles.empty()) {
      const uint32_t tile_count = static_cast<uint32_t>(strip_tiles.size());
      const uint32_t padding = stage_strip.padding_px;
      const uint32_t strip_height = stage_strip.strip_height_px;
      const uint32_t reserved_padding = padding * 2;
      const uint32_t usable_width =
          (swapchain.extent.width > reserved_padding) ? (swapchain.extent.width - reserved_padding)
                                                      : swapchain.extent.width;

      // Calculate max tiles per row based on available width (minimum 60px per tile)
      const uint32_t tile_width_min = 60;
      const uint32_t max_tiles_per_row =
          std::max<uint32_t>(usable_width / (tile_width_min + padding), 1);
      const uint32_t tiles_per_row = std::min(max_tiles_per_row, tile_count);
      const uint32_t tile_width = usable_width / tiles_per_row;
      const uint32_t tile_height =
          std::max<uint32_t>((strip_height > 2 * padding) ? (strip_height - 2 * padding)
                                                           : strip_height,
                             1);

      // Store original viewport for restoration
      const VkViewport original_viewport = viewport;

      const float tile_aspect = static_cast<float>(tile_width) / static_cast<float>(tile_height);
      const float scene_aspect = static_cast<float>(swapchain.extent.width) / static_cast<float>(swapchain.extent.height);

      for (uint32_t tile_idx = 0; tile_idx < tile_count; ++tile_idx) {
        uint32_t row = tile_idx / tiles_per_row;
        uint32_t col = tile_idx % tiles_per_row;

        const uint32_t tile_x = padding + col * (tile_width + padding);
        const uint32_t tile_y = padding + row * (tile_height + padding);
        const uint32_t clamped_width =
            std::min<uint32_t>(tile_width, swapchain.extent.width - std::min(tile_x, swapchain.extent.width));

        // Calculate aspect-ratio preserving viewport
        float viewport_width = static_cast<float>(clamped_width);
        float viewport_height = static_cast<float>(tile_height);
        float viewport_x = static_cast<float>(tile_x);
        float viewport_y = static_cast<float>(tile_y);

        // Fit scene content into tile with proper aspect ratio
        if (tile_aspect < scene_aspect) {
          // Tile is narrower than scene - constrain by width
          viewport_height = viewport_width / scene_aspect;
          viewport_y += (static_cast<float>(tile_height) - viewport_height) * 0.5f;
        } else {
          // Tile is wider than scene - constrain by height
          viewport_width = viewport_height * scene_aspect;
          viewport_x += (static_cast<float>(clamped_width) - viewport_width) * 0.5f;
        }

        VkViewport tile_viewport{.x = viewport_x,
                                 .y = viewport_y,
                                 .width = viewport_width,
                                 .height = viewport_height,
                                 .minDepth = 0.0f,
                                 .maxDepth = 1.0f};
        vkCmdSetViewport(command_buffer, 0, 1, &tile_viewport);

        VkRect2D tile_scissor{.offset = {static_cast<int32_t>(tile_x), static_cast<int32_t>(tile_y)},
                              .extent = {.width = clamped_width, .height = tile_height}};
        vkCmdSetScissor(command_buffer, 0, 1, &tile_scissor);

        const CE::RenderGUI::StageStripTile &tile_config = strip_tiles[tile_idx];
        for (const std::string &pipeline_name : tile_config.pipelines) {
          draw_pipeline_by_name(pipeline_name);
        }
      }

      vkCmdSetViewport(command_buffer, 0, 1, &original_viewport);
      vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    }
  }

  vkCmdEndRenderPass(command_buffer);

  //       This is part of an image memory barrier (i.e., vkCmdPipelineBarrier
  //       with the VkImageMemoryBarrier parameter set)

  std::vector<std::string> post_compute;
  if (graphics_render_graph) {
    for (const CE::Runtime::RenderNode &node : graphics_render_graph->nodes) {
      if (node.stage == CE::Runtime::RenderStage::PostCompute) {
        post_compute.push_back(node.pipeline);
      }
    }
  } else {
    if (legacy_plan) {
      post_compute = legacy_plan->post_graphics_compute;
    }
  }

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

    resources.push_constant.set_data(resources.world._time.passed_hours,
                     resources.world._time.get_day_fraction());
    vkCmdPushConstants(command_buffer,
                       pipelines.compute.layout,
                       resources.push_constant.shader_stage,
                       resources.push_constant.offset,
                       resources.push_constant.size,
                       resources.push_constant.data.data());

    const auto insert_compute_barrier = [&](VkCommandBuffer buffer) {
      VkMemoryBarrier barrier{};
      barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
      barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(buffer,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0,
                           1,
                           &barrier,
                           0,
                           nullptr,
                           0,
                           nullptr);
    };

    for (std::size_t i = 0; i < post_compute.size(); ++i) {
      const std::string &pipeline_name = post_compute[i];
      vkCmdBindPipeline(command_buffer,
                        VK_PIPELINE_BIND_POINT_COMPUTE,
                        pipelines.config.get_pipeline_object_by_name(pipeline_name));
      const std::array<uint32_t, 3> &work_groups =
          pipelines.config.get_work_groups_by_name(pipeline_name);
      vkCmdDispatch(command_buffer, work_groups[0], work_groups[1], work_groups[2]);
      if (i + 1 < post_compute.size()) {
        insert_compute_barrier(command_buffer);
      }
    }

    swapchain.images[image_index].transition_layout(command_buffer,
                                                    VK_FORMAT_R8G8B8A8_SRGB,
                                                    VK_IMAGE_LAYOUT_GENERAL,
                                                    /* -> */ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }

  CE::vulkan_result(vkEndCommandBuffer, command_buffer);
}
