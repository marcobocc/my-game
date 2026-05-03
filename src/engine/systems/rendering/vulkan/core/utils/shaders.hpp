#pragma once
#include <map>
#include <vector>
#include <volk.h>
#include "error_handling.hpp"
#include "spirv_reflect.h"

inline std::pair<VkShaderModule, VkPipelineShaderStageCreateInfo>
createShader(VkDevice device, const std::vector<char>& bytecode, VkShaderStageFlagBits stage) {
    VkShaderModuleCreateInfo moduleInfo{};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = bytecode.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

    VkShaderModule module = VK_NULL_HANDLE;
    throwIfUnsuccessful(vkCreateShaderModule(device, &moduleInfo, nullptr, &module), "Failed to create shader module");

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage;
    stageInfo.module = module;
    stageInfo.pName = "main";

    return {module, stageInfo};
}

inline uint32_t getPushConstantSize(const std::vector<char>& bytecode) {
    SpvReflectShaderModule module{};
    if (bytecode.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error("Invalid SPIR-V size");
    }
    std::vector<uint32_t> spirv(bytecode.size() / sizeof(uint32_t));
    std::memcpy(spirv.data(), bytecode.data(), bytecode.size());
    SpvReflectResult result = spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        return 0;
    }
    uint32_t count = 0;
    result = spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
    uint32_t size = 0;
    if (result == SPV_REFLECT_RESULT_SUCCESS && count > 0) {
        std::vector<SpvReflectBlockVariable*> blocks(count);
        spvReflectEnumeratePushConstantBlocks(&module, &count, blocks.data());
        size = blocks[0]->size; // first block only
    }
    spvReflectDestroyShaderModule(&module);
    return size;
}

inline std::vector<VkDescriptorSetLayout> reflectDescriptorSetLayouts(VkDevice device,
                                                                      const std::vector<char>& vertBytecode,
                                                                      const std::vector<char>& fragBytecode) {
    struct ReflectedBinding {
        VkDescriptorType descriptorType = {};
        VkShaderStageFlags stageFlags = 0;
    };
    struct StageSource {
        const std::vector<char>& bytecode;
        VkShaderStageFlagBits stage;
    };
    const StageSource sources[] = {{vertBytecode, VK_SHADER_STAGE_VERTEX_BIT},
                                   {fragBytecode, VK_SHADER_STAGE_FRAGMENT_BIT}};

    // set -> binding -> merged info
    std::map<uint32_t, std::map<uint32_t, ReflectedBinding>> sets;

    for (const auto& [bytecode, stage]: sources) {
        SpvReflectShaderModule spvModule{};
        std::vector<uint32_t> spirv(bytecode.size() / sizeof(uint32_t));
        std::memcpy(spirv.data(), bytecode.data(), bytecode.size());
        if (spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &spvModule) !=
            SPV_REFLECT_RESULT_SUCCESS)
            continue;

        uint32_t count = 0;
        spvReflectEnumerateDescriptorSets(&spvModule, &count, nullptr);
        std::vector<SpvReflectDescriptorSet*> spvSets(count);
        spvReflectEnumerateDescriptorSets(&spvModule, &count, spvSets.data());

        for (const auto* spvSet: spvSets) {
            auto& bindingMap = sets[spvSet->set];
            for (uint32_t b = 0; b < spvSet->binding_count; ++b) {
                const auto* spvBinding = spvSet->bindings[b];
                auto& entry = bindingMap[spvBinding->binding];
                entry.descriptorType = static_cast<VkDescriptorType>(spvBinding->descriptor_type);
                entry.stageFlags |= stage;
            }
        }
        spvReflectDestroyShaderModule(&spvModule);
    }

    std::vector<VkDescriptorSetLayout> layouts;
    layouts.reserve(sets.size());
    for (const auto& [setIndex, bindingMap]: sets) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(bindingMap.size());
        for (const auto& [bindingIndex, info]: bindingMap) {
            VkDescriptorSetLayoutBinding b{};
            b.binding = bindingIndex;
            b.descriptorType = info.descriptorType;
            b.descriptorCount = 1;
            b.stageFlags = info.stageFlags;
            bindings.push_back(b);
        }
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
        layouts.push_back(layout);
    }
    return layouts;
}
