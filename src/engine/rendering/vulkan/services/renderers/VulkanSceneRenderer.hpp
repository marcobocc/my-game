#pragma once
#include <vector>
#include "assets/AssetManager.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Mesh.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"

struct DrawCall {
    const Mesh& mesh;
    const Material& material;
    const Transform& transform;
};

class VulkanSceneRenderer {
public:
    VulkanSceneRenderer(const VulkanContext& vulkanContext,
                        VulkanResourcesManager& resourcesManager,
                        AssetManager& assetManager);

    ~VulkanSceneRenderer();

    void enqueueForDrawing(const Mesh&, const Material&, const Transform&);
    void drawScene(VkCommandBuffer cmd, const Camera& camera);

private:
    void createPerFrameUBO();
    void updatePerFrameUBO(const Camera& camera) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const;

    VulkanPerFrameUBO perFrameUBO_;
    VkDescriptorSetLayout perFrameUBOLayout_ = VK_NULL_HANDLE;
    std::vector<DrawCall> drawQueue_;

    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;
    VulkanContext& vulkanContext_;
};
