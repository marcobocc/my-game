#pragma once
#include <vector>
#include <volk.h>

inline VkDescriptorSet createEmptyDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    vkAllocateDescriptorSets(device, &info, &set);
    return set;
}

inline VkDescriptorSet createUniformBufferDescriptorSet(VkDevice device,
                                                        VkDescriptorPool pool,
                                                        VkDescriptorSetLayout layout,
                                                        VkBuffer buffer,
                                                        VkDeviceSize range,
                                                        uint32_t binding = 0) {
    VkDescriptorSet set = createEmptyDescriptorSet(device, pool, layout);

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = buffer;
    bufInfo.offset = 0;
    bufInfo.range = range;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufInfo;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    return set;
}
