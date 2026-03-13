#include "vulkan_renderer.h"

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
void VulkanRenderer::init(IWindow* window) {
    create_instance();
    setup_debug_messenger();
    create_surface(window->get_window());
    pick_physical_device();
    create_logical_device();
    create_swap_chain(window->get_window());
    create_image_views();
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
        required_layers.assign(_validation_layers.begin(), _validation_layers.end());
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
            std::cout << "Found a physical device: " << physical_device.getProperties().deviceName << std::endl;
            return;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU");
}

bool VulkanRenderer::is_device_suitable(vk::raii::PhysicalDevice const & physical_device) {
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

void VulkanRenderer::create_logical_device() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = _physical_device->getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            _physical_device->getSurfaceSupportKHR(qfpIndex, *_surface))
        {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},                                   // vk::PhysicalDeviceFeatures2
        {.dynamicRendering = true},           // vk::PhysicalDeviceVulkan13Features
        {.extendedDynamicState = true}        // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    };

    // create a Device
    float                     queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo      deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(required_device_extension.size()),
        .ppEnabledExtensionNames = required_device_extension.data()};

    _device = std::make_unique<vk::raii::Device>( *_physical_device, deviceCreateInfo );
    _graphics_queue = std::make_unique<vk::raii::Queue>(*_device, queueIndex, 0);
#ifndef NDEBUG
    std::cout << "Created Logical Device" << std::endl;
#endif
}

void VulkanRenderer::create_surface(GLFWwindow* window) {
    VkSurfaceKHR surface;
    if(glfwCreateWindowSurface(*(*_instance), window, nullptr, &surface)){
        throw std::runtime_error("failed to create window surface!");
    }
    _surface = std::make_unique<vk::raii::SurfaceKHR>(*_instance, surface);
#ifndef NDEBUG
    std::cout << "Created Surface" << std::endl;
#endif
}

vk::PresentModeKHR VulkanRenderer::choose_swap_present_mode(
    std::vector<vk::PresentModeKHR> const &availablePresentModes
) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
    return std::ranges::any_of(availablePresentModes,
                               [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
    vk::PresentModeKHR::eMailbox :
    vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanRenderer::choose_swap_extent(GLFWwindow* window, vk::SurfaceCapabilitiesKHR const &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void VulkanRenderer::create_swap_chain(GLFWwindow* window) {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities     = _physical_device->getSurfaceCapabilitiesKHR( *(*_surface) );
    _swap_chain_extent                                 = choose_swap_extent(window, surfaceCapabilities);
    uint32_t minImageCount                             = choose_swap_min_image_count(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = _physical_device->getSurfaceFormatsKHR(*(*_surface));
    _swap_chain_surface_format                         = choose_swap_surface_format(availableFormats);
    std::vector<vk::PresentModeKHR> availablePresentModes = _physical_device->getSurfacePresentModesKHR(*(*_surface));
    vk::PresentModeKHR              presentMode           = choose_swap_present_mode(availablePresentModes);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface          = *(*_surface),
        .minImageCount    = minImageCount,
        .imageFormat      = _swap_chain_surface_format.format,
        .imageColorSpace  = _swap_chain_surface_format.colorSpace,
        .imageExtent      = _swap_chain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = choose_swap_present_mode(availablePresentModes),
        .clipped          = true
    };
    _swap_chain        = vk::raii::SwapchainKHR(*_device, swapChainCreateInfo);
    _swap_chain_images = _swap_chain.getImages();

#ifndef NDEBUG
    std::cout << "Created Swap Chain" << std::endl;
#endif
}

uint32_t VulkanRenderer::choose_swap_min_image_count(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

vk::SurfaceFormatKHR VulkanRenderer::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

void VulkanRenderer::create_image_views() {
    assert(_swap_chain_image_views.empty());

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType         = vk::ImageViewType::e2D,
        .format           = _swap_chain_surface_format.format,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };

    for (auto &image : _swap_chain_images) {
        imageViewCreateInfo.image = image;
        _swap_chain_image_views.emplace_back( *_device, imageViewCreateInfo );
    }

#ifndef NDEBUG
    std::cout << "Created Image Views" << std::endl;
#endif
}
