#include "rendering/vulkan/services/VulkanImguiRenderer.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <volk.h>

VulkanImguiRenderer::VulkanImguiRenderer(const VulkanContext& vulkanContext,
                                         VulkanSwapchain& swapchain,
                                         uint32_t imageCount,
                                         GLFWwindow* window,
                                         UserInterface* userInterface) :
    swapchain_(swapchain),
    swapchainImageFormat_(swapchain.getSwapchainImageFormat()),
    userInterface_(userInterface) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = vulkanContext.getVkInstance();
    initInfo.PhysicalDevice = vulkanContext.getVkPhysicalDevice();
    initInfo.Device = vulkanContext.getVkDevice();
    initInfo.Queue = vulkanContext.getVkGraphicsQueue();
    initInfo.DescriptorPool = vulkanContext.getVkDescriptorPool();
    initInfo.MinImageCount = imageCount;
    initInfo.ImageCount = imageCount;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat_;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo);
}

VulkanImguiRenderer::~VulkanImguiRenderer() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanImguiRenderer::recordUIPass(VkCommandBuffer cmd, uint32_t imageIndex) {
    beginRendering(cmd, imageIndex);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (const auto& widget: userInterface_->widgets())
        widget->draw();
    ImGui::Render();
    if (ImDrawData* drawData = ImGui::GetDrawData()) {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
    }
    endRendering(cmd);
}

void VulkanImguiRenderer::beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchain_.getVkImageView(imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = {};
    colorAttachment.pNext = nullptr;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea.offset = {0, 0};
    renderInfo.renderArea.extent = swapchain_.getVkExtent();
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = nullptr;
    renderInfo.pStencilAttachment = nullptr;
    renderInfo.pNext = nullptr;

    vkCmdBeginRendering(cmd, &renderInfo);
}

void VulkanImguiRenderer::endRendering(VkCommandBuffer cmd) { vkCmdEndRendering(cmd); }
