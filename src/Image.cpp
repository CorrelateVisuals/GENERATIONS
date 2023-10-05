#include "Image.h"

#include <iostream>

VkDevice* Image::_logicalDevice = nullptr;

Image::Image()
    : image{},
      imageMemory{},
      imageView{},
      imageSampler{},
      sampleCount{VK_SAMPLE_COUNT_1_BIT} {}

Image::~Image() {
  if (*_logicalDevice != VK_NULL_HANDLE) {
    std::cout << "DEVICE EXISTS ";
    std::cout << imageMemory << " " << image << " " << imageView << " "
              << imageSampler << std::endl;

    if (imageSampler != VK_NULL_HANDLE) {
      std::cout << "imageSampler  ";
      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
      vkDestroySampler(*_logicalDevice, imageSampler, nullptr);
      imageSampler = VK_NULL_HANDLE;
      std::cout << "          ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
    };
    if (imageView != VK_NULL_HANDLE) {
      std::cout << "imageView   ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
      vkDestroyImageView(*_logicalDevice, imageView, nullptr);
      imageView = VK_NULL_HANDLE;
      std::cout << "          ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
    };
    if (image != VK_NULL_HANDLE) {
      std::cout << "image        ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
      vkDestroyImage(*_logicalDevice, image, nullptr);
      image = VK_NULL_HANDLE;
      std::cout << "          ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
    };
    if (imageMemory != VK_NULL_HANDLE) {
      std::cout << "imageMemory  ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
      vkFreeMemory(*_logicalDevice, imageMemory, nullptr);
      imageMemory = VK_NULL_HANDLE;
      std::cout << "          ";

      std::cout << imageMemory << " " << image << " " << imageView << " "
                << imageSampler << std::endl;
    };
  }
}
