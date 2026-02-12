#pragma once

/**
 * @file BaseClasses.h
 * @brief Unified header for Vulkan base classes
 * 
 * This file provides backward compatibility by including all the segmented
 * Vulkan base class headers. The classes have been organized into separate
 * files for better code organization and maintainability.
 * 
 * Organization:
 * - VulkanDevice.h: Device and queue management
 * - VulkanResources.h: Buffer and image management
 * - VulkanPipeline.h: Pipeline and render pass management
 * - VulkanSync.h: Synchronization and swapchain
 * - VulkanDescriptor.h: Descriptor set management
 * - VulkanUtils.h: Utility functions
 * - RenderInterface.h: API-agnostic rendering interface
 */

#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanPipeline.h"
#include "VulkanSync.h"
#include "VulkanDescriptor.h"
#include "VulkanUtils.h"

// Keep backward compatibility with enum
using IMAGE_RESOURCE_TYPES = enum IMAGE_RESOURCE_TYPES;

// Forward declarations
class VulkanMechanics;
class Resources;
class Pipelines;
