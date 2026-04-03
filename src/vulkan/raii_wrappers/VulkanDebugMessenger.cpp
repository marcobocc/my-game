#include "vulkan/raii_wrappers/VulkanDebugMessenger.hpp"
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"

VulkanDebugMessenger::~VulkanDebugMessenger() {
    if (debugMessenger_ != VK_NULL_HANDLE) vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
}

VulkanDebugMessenger::VulkanDebugMessenger(VkInstance instance) : instance_(instance) {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;
    info.pUserData = nullptr;

    const auto result = vkCreateDebugUtilsMessengerEXT(instance_, &info, nullptr, &debugMessenger_);
    throwIfUnsuccessful(result);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessenger::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                                   VkDebugUtilsMessageTypeFlagsEXT type,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                                   void* /*userData*/) {
    std::string severityStr;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        severityStr = "VERBOSE";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severityStr = "INFO";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severityStr = "WARNING";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severityStr = "ERROR";
    else
        severityStr = "UNKNOWN";

    std::string typeStr;
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) typeStr += "GENERAL ";
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) typeStr += "VALIDATION ";
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) typeStr += "PERFORMANCE ";

    const char* msg = (data && data->pMessage) ? data->pMessage : "<no message>";
    std::string message = "[Vulkan][" + severityStr + "] [" + typeStr + "] " + msg;

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LOG4CXX_ERROR(LOGGER, message);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LOG4CXX_WARN(LOGGER, message);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        LOG4CXX_INFO(LOGGER, message);
    else
        LOG4CXX_DEBUG(LOGGER, message);

    return VK_FALSE;
}
