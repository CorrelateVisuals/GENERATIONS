#pragma once
#include <glm/glm.hpp>

#include "BaseClasses.h"
#include "Library.h"
#include "World.h"

class VulkanMechanics;
class Resources;

class Pipelines {
 public:
  Pipelines(VulkanMechanics& mechanics, Resources& resources);
  ~Pipelines();

  struct ComputeLayout : public CE::PipelineLayout {
    ComputeLayout(CE::DescriptorInterface& interface,
                  CE::PushConstants& pushConstant) {
      createLayout(interface.setLayout, pushConstant);
    }
  };

  struct GraphicsLayout : public CE::PipelineLayout {
    GraphicsLayout(CE::DescriptorInterface& interface) {
      createLayout(interface.setLayout);
    }
  };

  struct Render : public CE::RenderPass {
    Render(CE::Swapchain& swapchain,
           const CE::Image& msaaImage,
           const VkImageView& depthView) {
      create(msaaImage.info.samples, swapchain.imageFormat);
      createFramebuffers(swapchain, msaaImage.view, depthView);
    }
  };

  struct Configuration : public CE::PipelinesConfiguration {
    Configuration(VkRenderPass& renderPass,
                  const VkPipelineLayout& graphicsLayout,
                  const VkPipelineLayout& computeLayout,
                  VkSampleCountFlagBits& msaaSamples) {
      pipelineMap["Triangle"] = Graphics{.shaders = {"Vert", "Frag"}};

      compileShaders();
      createPipelines(renderPass, graphicsLayout, computeLayout, msaaSamples);
    }
  };

  ComputeLayout compute;
  GraphicsLayout graphics;
  Render render;
  Configuration config;
};
