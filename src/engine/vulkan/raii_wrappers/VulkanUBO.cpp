#include "vulkan/raii_wrappers/VulkanUBO.hpp"
#include <vector>
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"

VulkanUBO::VulkanUBO(const VulkanContext& vulkanContext, size_t uboSize) :
    device_(vulkanContext.getVkDevice()),
    physicalDevice_(vulkanContext.getVkPhysicalDevice()),
    uboSize_(uboSize) {
    createDescriptorSetLayout();
    createBuffers();
    createDescriptorSets();
}

VulkanUBO::~VulkanUBO() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (buffers_[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device_, buffers_[i], nullptr);
            }
            if (bufferMemories_[i] != VK_NULL_HANDLE) {
                vkFreeMemory(device_, bufferMemories_[i], nullptr);
            }
        }
        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        }
        if (descriptorSetLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        }
    }
}

void VulkanUBO::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    throwIfUnsuccessful(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_));
}

void VulkanUBO::createBuffers() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDeviceSize bufferSize = uboSize_;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        throwIfUnsuccessful(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffers_[i]));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device_, buffers_[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanBuffer::getMemoryType(physicalDevice_,
                                                                 memRequirements.memoryTypeBits,
                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        throwIfUnsuccessful(vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemories_[i]));
        throwIfUnsuccessful(vkBindBufferMemory(device_, buffers_[i], bufferMemories_[i], 0));
    }
}

void VulkanUBO::createDescriptorSets() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    throwIfUnsuccessful(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_));

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    throwIfUnsuccessful(vkAllocateDescriptorSets(device_, &allocInfo, descriptorSets_.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffers_[i];
        bufferInfo.offset = 0;
        bufferInfo.range = uboSize_;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets_[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
    }
}

void VulkanUBO::updateUBO(size_t frameIndex, const void* uboData, size_t uboSize) const {
    void* data;
    vkMapMemory(device_, bufferMemories_[frameIndex], 0, uboSize, 0, &data);
    memcpy(data, uboData, uboSize);
    vkUnmapMemory(device_, bufferMemories_[frameIndex]);
}
