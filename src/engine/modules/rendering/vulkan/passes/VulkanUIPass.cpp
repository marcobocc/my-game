#include "VulkanUIPass.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "../core/VulkanSwapchainManager.hpp"
#include "modules/core/GameWindow.hpp"

VulkanUIPass::VulkanUIPass(const VulkanContext& vulkanContext,
                           VulkanSwapchainManager& swapchainManager,
                           GameWindow& window,
                           UserInterface& userInterface) :
    userInterface_(userInterface) {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplGlfw_InitForVulkan(window.get(), true);

    ApplyEditorStyle();

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

void VulkanUIPass::record(VkCommandBuffer cmd, VkImageView colorView, VkExtent2D extent) {
    beginRendering(cmd, colorView, extent);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (const auto& widget: userInterface_.widgets())
        widget->draw();
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

void VulkanUIPass::ApplyEditorStyle() const {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 2);
    style.ItemSpacing = ImVec2(6, 3);
    style.ItemInnerSpacing = ImVec2(4, 2);
    style.IndentSpacing = 16.0f;
    style.FrameRounding = 4.0f;
    style.WindowRounding = 0.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.65f, 0.25f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
    colors[ImGuiCol_Header] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.50f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95f, 0.65f, 0.25f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.82f, 0.55f, 0.20f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.82f, 0.55f, 0.20f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
