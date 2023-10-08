#pragma once
#include <vulkan/vulkan.h>

#include <vector>

class CE {
public:
    static VkDevice* _logicalDevice;
    static void setLogicalDevice(VkDevice* logicalDevice);

    class Buffer {
    public:
        Buffer();
        virtual ~Buffer();

        VkBuffer buffer;
        VkDeviceMemory bufferMemory;
        void* mapped;
    };

    class Image {
    public:
        Image();
        virtual ~Image();

        VkImage image;
        VkDeviceMemory imageMemory;
        VkImageView imageView;
        VkSampler imageSampler;
        VkSampleCountFlagBits sampleCount;
    };

    struct DescriptorSetLayout {
        VkDescriptorSetLayoutBinding layoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL };
    };

    class Descriptors {
    public:
        Descriptors();
        ~Descriptors();

        VkDescriptorPool pool;
        VkDescriptorSetLayout setLayout;
        std::vector<VkDescriptorSet> sets;

    private:
        void createDescriptorPool();
        void allocateDescriptorSets();
        void createDescriptorSets();
    };

};
