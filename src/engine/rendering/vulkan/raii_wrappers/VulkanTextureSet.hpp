#pragma once
#include <vulkan/vulkan.h>

class VulkanTextureSet {
public:
    VulkanTextureSet(VkDevice device, VkDescriptorPool pool);
    ~VulkanTextureSet();
    VkDescriptorSetLayout getLayout() const;
    VkDescriptorSet allocateSet();
    void updateSet(VkDescriptorSet set, VkImageView imageView, VkSampler sampler);

private:
    VkDevice device_;
    VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_;
};
