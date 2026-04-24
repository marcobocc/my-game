#pragma once
#include <optional>
#include <string>

struct VulkanContext;
class VulkanRenderingOrchestrator;
struct Renderer;
struct Transform;
struct Camera;

/*
    VulkanGraphicsBackend (Facade)

    Purpose:
    --------------------------------------------------
    High-level facade for the Vulkan rendering backend.

    Responsibilities:
    --------------------------------------------------
    - Provide a simple, stable API for rendering operations
    - Coordinate calls to underlying Vulkan systems
    - Hide internal rendering complexity from the application layer

    This class is not responsible for:
    --------------------------------------------------
    - Creating or owning Vulkan subsystems
    - Managing dependency lifetimes
    - Low-level Vulkan setup or resource allocation
    - Application logic or game/engine orchestration

    Notes:
    --------------------------------------------------
    This class is intentionally thin and delegates work to
    injected renderer/swapchain systems.

    It should remain stable and minimal. If it starts accumulating
    construction logic or subsystem wiring, it is violating
    the facade responsibility and should be refactored.
*/
class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    ~VulkanGraphicsBackend();
    VulkanGraphicsBackend(VulkanContext& context, VulkanRenderingOrchestrator& renderer);

    void draw(const Renderer& renderer, const Transform& transform, std::string objectId) const;
    void renderFrame(const Camera& camera, const Transform& cameraTransform) const;
    void requestPick(uint32_t x, uint32_t y) const;
    std::optional<std::string> getPickResult() const;

private:
    VulkanContext& context_;
    VulkanRenderingOrchestrator& renderer_;
};
