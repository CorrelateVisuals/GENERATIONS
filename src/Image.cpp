#include "Image.h"

Image::Image()
    : image{},
      imageMemory{},
      imageView{},
      imageSampler{},
      samples{VK_SAMPLE_COUNT_1_BIT} {}

Image::~Image(){
    vkDestroySampler(*_logicalDevice, imageSampler, nullptr);
    vkDestroyImageView(*_logicalDevice, imageView, nullptr);
    vkDestroyImage(*_logicalDevice, image, nullptr);
    vkFreeMemory(*_logicalDevice, imageMemory, nullptr);
}
