#pragma once

#include <cstddef>
#include <cstdint>

enum IMAGE_RESOURCE_TYPES { CE_DEPTH_IMAGE = 0, CE_MULTISAMPLE_IMAGE = 1 };

namespace {
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr size_t NUM_DESCRIPTORS = 5;
}  // namespace
