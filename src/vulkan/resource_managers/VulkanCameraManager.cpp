#include "vulkan/resource_managers/VulkanCameraManager.hpp"
#include <vector>
#include <volk.h>
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/VulkanErrorHandling.hpp"

VulkanCameraManager::VulkanCameraManager(VkDevice device, VkPhysicalDevice physicalDevice) :
    device_(device),
    physicalDevice_(physicalDevice) {
    createDescriptorSetLayout();
    createBuffers();
    createDescriptorSets();
}

VulkanCameraManager::~VulkanCameraManager() {
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

void VulkanCameraManager::createDescriptorSetLayout() {
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

void VulkanCameraManager::createBuffers() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDeviceSize bufferSize = sizeof(CameraUBO);
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
        allocInfo.memoryTypeIndex = VulkanBuffer::findMemoryType(physicalDevice_,
                                                                 memRequirements.memoryTypeBits,
                                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        throwIfUnsuccessful(vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemories_[i]));
        throwIfUnsuccessful(vkBindBufferMemory(device_, buffers_[i], bufferMemories_[i], 0));
    }
}

void VulkanCameraManager::createDescriptorSets() {
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
        bufferInfo.range = sizeof(CameraUBO);

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

void VulkanCameraManager::updateCameraUBO(size_t frameIndex, const CameraComponent& camera) const {
    CameraUBO ubo;
    ubo.view = camera.getViewMatrix();
    ubo.proj = camera.getProjectionMatrix();

    void* data;
    vkMapMemory(device_, bufferMemories_[frameIndex], 0, sizeof(CameraUBO), 0, &data);
    memcpy(data, &ubo, sizeof(CameraUBO));
    vkUnmapMemory(device_, bufferMemories_[frameIndex]);
}
