#include "vulkan_renderer.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

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
    pick_physical_device();
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

void VulkanRenderer::pick_physical_device() {
    auto physical_devices = _instance->enumeratePhysicalDevices();

    if (physical_devices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

#ifndef NDEBUG
    std::cout << "Founded physical devices: " << physical_devices.size() << "\n";
#endif

    for (auto physical_device : physical_devices) {
#ifndef NDEBUG
        std::cout << "Validating a physical device: " << physical_device.getProperties().deviceName << std::endl;
#endif
        if (is_device_suitable(physical_device)) {
            _physical_device = std::make_unique<vk::raii::PhysicalDevice>(physical_device);
            std::cout << "Found a physical device: " << physical_device.getProperties().deviceName;
            return;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU");
}

bool VulkanRenderer::is_device_suitable(vk::raii::PhysicalDevice const & physical_device) {
    std::vector<const char*> required_device_extension = {vk::KHRSwapchainExtensionName};

    bool supports_vulkan_1_3 = physical_device.getProperties().apiVersion >= vk::ApiVersion13;

    auto queue_families = physical_device.getQueueFamilyProperties();
    bool supports_graphics =
        std::ranges::any_of(queue_families, [](auto const &qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

    auto available_device_extensions = physical_device.enumerateDeviceExtensionProperties();
    bool supports_all_required_extensions =
        std::ranges::all_of(required_device_extension,
                            [&available_device_extensions]( auto const & required_device_extension )
                            {
                            return std::ranges::any_of( available_device_extensions,
                                                       [required_device_extension]( auto const & available_device_extension )
                                                       { return strcmp( available_device_extension.extensionName, required_device_extension ) == 0; } );
                            } );

    auto features =
        physical_device
        .template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool supports_required_features = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

    return supports_vulkan_1_3 && supports_graphics && supports_required_features && supports_all_required_extensions;
}
