#include "VulkanUIPass.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "../core/VulkanSwapchainManager.hpp"
#include "modules/core/GameWindow.hpp"

VulkanUIPass::VulkanUIPass(const VulkanContext& vulkanContext,
                           VulkanSwapchainManager& swapchainManager,
                           GameWindow& window) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplGlfw_InitForVulkan(window.get(), true);

    VkFormat swapchainImageFormat = swapchainManager.swapchain().swapchainImageFormat;

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = vulkanContext.instance;
    initInfo.PhysicalDevice = vulkanContext.physicalDevice;
    initInfo.Device = vulkanContext.device;
    initInfo.Queue = vulkanContext.graphicsQueue;
    initInfo.DescriptorPool = vulkanContext.descriptorPool;
    initInfo.MinImageCount = static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size());
    initInfo.ImageCount = static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size());
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo);
}

VulkanUIPass::~VulkanUIPass() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanUIPass::setDrawCallback(std::function<void()> callback) { drawCallback_ = std::move(callback); }

void VulkanUIPass::record(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent) {
    beginRendering(cmd, colorView, extent);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (drawCallback_) drawCallback_();
    ImGui::Render();
    if (ImDrawData* drawData = ImGui::GetDrawData()) {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
    }
    endRendering(cmd);
}

void VulkanUIPass::beginRendering(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent) const {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = colorView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = {};
    colorAttachment.pNext = nullptr;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.renderArea.extent = extent;
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = nullptr;
    renderInfo.pStencilAttachment = nullptr;
    renderInfo.pNext = nullptr;

    vkCmdBeginRendering(cmd, &renderInfo);
}

void VulkanUIPass::endRendering(VkCommandBuffer cmd) { vkCmdEndRendering(cmd); }
