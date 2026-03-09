#include "vulkan_renderer.h"

#include <GLFW/glfw3.h>
#include <iostream>

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::cerr << "validation layer: type " << type
              << " msg: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}
void VulkanRenderer::init() {
    create_instance();
    setup_debug_messenger();
    return;
}

void VulkanRenderer::draw_triangle() {
    return;
}

void VulkanRenderer::draw_rectangle() {
    return;
}

std::vector<const char*> VulkanRenderer::get_required_instance_extensions()
{
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (enable_validation_layers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    return extensions;
}

void VulkanRenderer::create_instance() {
    // Get the required layers
    std::vector<char const*> required_layers;
    if (enable_validation_layers) {
        required_layers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layer_properties = _context->enumerateInstanceLayerProperties();
    auto unsupported_layer_it = std::ranges::find_if(required_layers,
                                                   [&layer_properties](auto const &requiredLayer) {
                                                   return std::ranges::none_of(layer_properties,
                                                                               [requiredLayer](auto const &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
                                                   });

    if (unsupported_layer_it != required_layers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupported_layer_it));
    }


    constexpr vk::ApplicationInfo app_info{.pApplicationName   = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion         = vk::ApiVersion13};

    auto required_extensions = get_required_instance_extensions();

    auto extensionProperties = _context->enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < required_extensions.size(); ++i)
    {
        if (std::ranges::none_of(extensionProperties,
                                 [extension = required_extensions[i]](auto const& extensionProperty)
                                 { return strcmp(extensionProperty.extensionName, extension) == 0; }))
        {
            throw std::runtime_error("Required extension not supported: " + std::string(required_extensions[i]));
        }
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames = required_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()};

    _instance = std::make_unique<vk::raii::Instance>(*_context, createInfo);
}

void VulkanRenderer::setup_debug_messenger() {
    if (!enable_validation_layers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
        .messageType     = messageTypeFlags,
        .pfnUserCallback = &VulkanRenderer::debug_callback};
    *_debug_messenger = _instance->createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
}
