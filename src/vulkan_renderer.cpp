#include "vulkan_renderer.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/trigonometric.hpp"
#include "loader.h"
#include "util.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <glm/ext/matrix_transform.hpp>
#include <cstring>
#include <algorithm>

VulkanRenderer::~VulkanRenderer() {
    std::cout << "VulkanRenderer Destructor called" << std::endl;

    if (_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
    }

    cleanup_imgui();


    // Descriptor pools
    if (_imgui_descriptor_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(_device, _imgui_descriptor_pool, nullptr);
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(_device, descriptorPool, nullptr);
    }


    // Pipeline
    if (_graphics_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
    }
    vkDestroyPipelineLayout(_device, pipelineLayout, nullptr);


    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            _device,
            descriptorSetLayout,
            nullptr
        );
    }


    // Buffers + memory
    vkDestroyBuffer(_device, _frame_z_data, nullptr);
    vkFreeMemory(_device, _frame_z_data_mem, nullptr);

    vkDestroyBuffer(_device, _staging_buffer, nullptr);
    vkFreeMemory(_device, _staging_buffer_mem, nullptr);

    vkDestroyBuffer(_device, _bathymetry_z_data, nullptr);
    vkFreeMemory(_device, _bathymetry_z_data_mem, nullptr);

    vkDestroyBuffer(_device, _grid_xy_data, nullptr);
    vkFreeMemory(_device, _grid_xy_data_mem, nullptr);

    vkDestroyBuffer(_device, _index_data, nullptr);
    vkFreeMemory(_device, _index_data_mem, nullptr);

    vkDestroyBuffer(_device, uniformBuffer, nullptr);
    vkFreeMemory(_device, uniformBufferMemory, nullptr);

    // Synchronization objects
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        if (_draw_fences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(
                _device,
                _draw_fences[i],
                nullptr
            );
        }

        if (_present_complete_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(
                _device,
                _present_complete_semaphores[i],
                nullptr
            );
        }

        if (_render_finished_semaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(
                _device,
                _render_finished_semaphores[i],
                nullptr
            );
        }
    }


    // Command pools
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        if (_command_pools[i] != VK_NULL_HANDLE) {
            vkDestroyCommandPool(
                _device,
                _command_pools[i],
                nullptr
            );
        }
    }

    // Depth Image View
    vkDestroyImageView(_device, _depth_image_view, nullptr);

    // Depth Image
    vkFreeMemory(_device, _depth_image_memory, nullptr);
    vkDestroyImage(_device, _depth_image, nullptr);


    // Swapchain image views
    for (auto view : _swap_chain_image_views) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(
                _device,
                view,
                nullptr
            );
        }
    }

    if (_swap_chain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            _device,
            _swap_chain,
            nullptr
        );
    }


    vkDestroySurfaceKHR(_instance, _surface, nullptr);

    if (_device != VK_NULL_HANDLE) {
        vkDestroyDevice(
            _device,
            nullptr
        );
    }

    vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);

    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(
            _instance,
            nullptr
        );
    }
}

VkBool32 VulkanRenderer::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData
)
{
    std::cerr << "validation layer: "
              << data->pMessage
              << std::endl;

    return VK_FALSE;
}

void VulkanRenderer::init(
    IWindow* window, 
    const std::vector<glm::vec2>& grid,
    const std::vector<float>& bathymetryZ,
    const std::vector<Triangle>& triangles,
    float x_extent,
    float y_extent
) {
    _window = window->get_window();

    if (volkInitialize() != VK_SUCCESS) {
        throw std::runtime_error("Failed to initalized volk");
    }

    create_instance();

    // Load instance function pointers
    volkLoadInstance(_instance);

    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_swap_chain();
    create_image_views();

    create_depth_image();
    create_depth_image_view();
    
    create_descriptor_set_layout();

    create_graphics_pipeline();
    create_command_pools();
    create_command_buffers();
    create_sync_objects();

    create_buffers(
        grid,
        bathymetryZ,
        triangles
    );

    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();

    init_imgui();

    // Configurar posicao inicial da camera
    float xcenter{x_extent / 2};
    float ycenter{y_extent / 2};
    
    camera.setTarget(glm::vec3{xcenter, ycenter, 0});

    return;
}

void VulkanRenderer::draw(AppInfo& info) {
#ifdef NDEBUG
    std::cerr << "Started draw\n";
#endif

    vkWaitForFences(_device, 1, &_draw_fences[_current_frame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex{};
    VkResult result = vkAcquireNextImageKHR(_device, _swap_chain, UINT64_MAX, _present_complete_semaphores[_current_frame], nullptr, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        _framebuffer_resized = false;
        recreate_swap_chain();
    }

    record_command_buffer(imageIndex, info);

    vkResetFences(_device, 1, &_draw_fences[_current_frame]);

    VkPipelineStageFlags waitDestinationStageMask( VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT );
    VkSubmitInfo submitInfo{};

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1,
    submitInfo.pWaitSemaphores = &_present_complete_semaphores[_current_frame];
    submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_command_buffers[_current_frame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_render_finished_semaphores[_current_frame];

    vkQueueSubmit(_graphics_queue, 1, &submitInfo, _draw_fences[_current_frame]);

    VkPresentInfoKHR presentInfoKHR{};
    presentInfoKHR.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfoKHR.waitSemaphoreCount = 1;
    presentInfoKHR.pWaitSemaphores = &_render_finished_semaphores[_current_frame];
    presentInfoKHR.swapchainCount = 1;
    presentInfoKHR.pSwapchains = &_swap_chain;
    presentInfoKHR.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(_graphics_queue, &presentInfoKHR);
    switch (result)
    {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
            std::cout << "Suboptimal KHR!\n";
            break;
        default:
            break;
    }

    _current_frame = (_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

#ifdef NDEBUG
    std::cerr << "Ended draw\n";
#endif
}

std::vector<const char*> VulkanRenderer::get_required_instance_extensions()
{
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

void VulkanRenderer::create_instance() {
    // Get the required layers
    std::vector<char const*> required_layers;
    if (enable_validation_layers) {
        required_layers.assign(_validation_layers.begin(), _validation_layers.end());
    }

    // Get instance layer properties
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layer_properties(count);
    vkEnumerateInstanceLayerProperties(&count, layer_properties.data());

    // Check if the required layers are supported by the Vulkan implementation.
    auto unsupported_layer_it = required_layers.end();

    for (auto it = required_layers.begin(); it != required_layers.end(); ++it) {
        bool found = false;

        for (const auto& layerProperty : layer_properties)
        {
            if (strcmp(layerProperty.layerName, *it) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            unsupported_layer_it = it;
            break;
        }
    }

    if (unsupported_layer_it != required_layers.end()) {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupported_layer_it));
    }


    constexpr VkApplicationInfo app_info{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
        .apiVersion         = VK_API_VERSION_1_3
    };

    auto required_extensions = get_required_instance_extensions();

    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensionProperties(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensionProperties.data());
    
    for (uint32_t i = 0; i < required_extensions.size(); ++i) {
        bool found = false;

        for (const auto& extensionProperty : extensionProperties) {
            if (strcmp(extensionProperty.extensionName,
                    required_extensions[i]) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error(
                "Required extension not supported: " +
                std::string(required_extensions[i]));
        }
    }

    VkInstanceCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(required_layers.size()),
        .ppEnabledLayerNames = required_layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()
    };

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance");
    }
}

void VulkanRenderer::setup_debug_messenger() {
    if (!enable_validation_layers) return;

    VkDebugUtilsMessageSeverityFlagsEXT  severityFlags = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    VkDebugUtilsMessageTypeFlagsEXT messageTypeFlags =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &VulkanRenderer::debug_callback
    };

    if(vkCreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug messenger");
    }
}

void VulkanRenderer::pick_physical_device() {
    uint32_t count{};
    vkEnumeratePhysicalDevices(_instance, &count, nullptr);
    std::vector<VkPhysicalDevice> physical_devices(count);
    vkEnumeratePhysicalDevices(_instance, &count, physical_devices.data());

    if (physical_devices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

#ifndef NDEBUG
    std::cout << "Founded physical devices: " << physical_devices.size() << "\n";
#endif

    for (auto physical_device : physical_devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physical_device, &props);

#ifndef NDEBUG
        std::cout << "Validating a physical device: " << props.deviceName << std::endl;
#endif
        if (is_device_suitable(physical_device)) {
            _physical_device = physical_device;
            std::cout << "Found a physical device: " << props.deviceName << std::endl;
            return;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU");
}

bool VulkanRenderer::is_device_suitable(VkPhysicalDevice const &physical_device) {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physical_device, &props);

    bool supports_vulkan_1_3 = props.apiVersion >= VK_API_VERSION_1_3;

    uint32_t count{};
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_families.data());

    bool supports_graphics = false;

    for (const auto& qfp : queue_families) {
        if (qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            supports_graphics = true;
            break;
        }
    }


    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available_device_extensions(count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, available_device_extensions.data());    

    bool supports_all_required_extensions = true;

    for (const auto& required_extension : required_device_extension) {
        bool found = false;

        for (const auto& available_extension : available_device_extensions) {
            if (strcmp(
                    available_extension.extensionName,
                    required_extension) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            supports_all_required_extensions = false;
            break;
        }
    }

    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr
    };

    VkPhysicalDeviceVulkan13Features vulkan13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr
    };

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = nullptr
    };

    features2.pNext = &vulkan13Features;
    vulkan13Features.pNext = &extendedDynamicStateFeatures;

    vkGetPhysicalDeviceFeatures2(
        physical_device,
        &features2
    );

    bool supports_required_features =
    vulkan13Features.dynamicRendering &&
    vulkan13Features.synchronization2 &&
    extendedDynamicStateFeatures.extendedDynamicState;

    return supports_vulkan_1_3 && supports_graphics && supports_required_features && supports_all_required_extensions;
}
void VulkanRenderer::create_logical_device() {
    uint32_t count{};
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &count, queueFamilyProperties.data());

    // get the first index into queueFamilyProperties which supports both graphics and present
    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        VkBool32 supportKHR{};
        vkGetPhysicalDeviceSurfaceSupportKHR(_physical_device, qfpIndex, _surface, &supportKHR);

        if (queueFamilyProperties[qfpIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT && supportKHR) {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr,
    };

    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT featuresExtended{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = nullptr,
        .extendedDynamicState = VK_TRUE,
    };

    features2.pNext = &features13;
    features13.pNext = &featuresExtended;


    // create a Device
    float queuePriority = 0.5f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, 
        .queueFamilyIndex = queueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkDeviceCreateInfo      deviceCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &features2,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = static_cast<uint32_t>(required_device_extension.size()),
        .ppEnabledExtensionNames = required_device_extension.data()
    };


    if (vkCreateDevice(_physical_device, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    } 

    _graphics_index = queueIndex;

    vkGetDeviceQueue(_device, _graphics_index, 0, &_graphics_queue);
#ifndef NDEBUG
    std::cout << "Created Logical Device" << std::endl;
#endif
}

void VulkanRenderer::create_surface() {
    if(glfwCreateWindowSurface(_instance, _window, nullptr, &_surface)){
        throw std::runtime_error("failed to create window surface!");
    }

#ifndef NDEBUG
    std::cout << "Created Surface" << std::endl;
#endif
}

VkPresentModeKHR VulkanRenderer::choose_swap_present_mode(
    std::vector<VkPresentModeKHR> const &availablePresentModes
) {
    bool mailboxSupported = false;
    bool fifoSupported = false;

    for (VkPresentModeKHR mode : availablePresentModes) {
        if (mode == VK_PRESENT_MODE_FIFO_KHR) {
            fifoSupported = true;
        }

        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            mailboxSupported = true;
        }
    }

    assert(fifoSupported);

    return mailboxSupported
            ? VK_PRESENT_MODE_MAILBOX_KHR
            : VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::choose_swap_extent(GLFWwindow* window, VkSurfaceCapabilitiesKHR const &capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void VulkanRenderer::create_swap_chain() {
    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &surfaceCapabilities)) {
        throw std::runtime_error("Failed to get surface caps");
    }

    _swap_chain_extent = choose_swap_extent(_window, surfaceCapabilities);
    uint32_t minImageCount = choose_swap_min_image_count(surfaceCapabilities);

    uint32_t count{};
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &count, availableFormats.data());

    _swap_chain_surface_format = choose_swap_surface_format(availableFormats);
    
    
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &count, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &count, availablePresentModes.data());

    VkPresentModeKHR presentMode = choose_swap_present_mode(availablePresentModes);

    VkSwapchainCreateInfoKHR swapChainCreateInfo {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = _surface,
        .minImageCount    = minImageCount,
        .imageFormat      = _swap_chain_surface_format.format,
        .imageColorSpace  = _swap_chain_surface_format.colorSpace,
        .imageExtent      = _swap_chain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = choose_swap_present_mode(availablePresentModes),
        .clipped          = true
    };

    if (vkCreateSwapchainKHR(_device, &swapChainCreateInfo, nullptr, &_swap_chain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }

    vkGetSwapchainImagesKHR(_device, _swap_chain, &count, nullptr);
    _swap_chain_images.resize(count);
    vkGetSwapchainImagesKHR(_device, _swap_chain, &count, _swap_chain_images.data());

#ifndef NDEBUG
    std::cout << "Created Swap Chain" << std::endl;
#endif
}

uint32_t VulkanRenderer::choose_swap_min_image_count(VkSurfaceCapabilitiesKHR const &surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

VkSurfaceFormatKHR VulkanRenderer::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return availableFormats[0];
}

void VulkanRenderer::create_image_views() {
    assert(_swap_chain_image_views.empty());

    VkImageViewCreateInfo imageViewCreateInfo{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = _swap_chain_surface_format.format,
        .subresourceRange = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            1,
            0,
            1
        }
    };

    _swap_chain_image_views.reserve(_swap_chain_images.size());

    for (auto& image : _swap_chain_images)
    {
        imageViewCreateInfo.image = image;

        VkImageView imageView;

        if (vkCreateImageView(
                _device,
                &imageViewCreateInfo,
                nullptr,
                &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image view");
        }

        _swap_chain_image_views.push_back(imageView);
    }

#ifndef NDEBUG
    std::cout << "Created Image Views" << std::endl;
#endif
}

void VulkanRenderer::create_graphics_pipeline() {
    auto shaderCode = read_file(std::filesystem::path(SHADERS_DIR) / "slang.spv");

    VkShaderModule shaderModule = create_shader_module(shaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{ 
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shaderModule,
        .pName = "vertMain"
    };
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shaderModule,
        .pName = "fragMain"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    std::array<VkVertexInputBindingDescription, 2> bindings{};

    // Binding for the grid_xy_data
    bindings[0].binding = 0;
    bindings[0].stride = sizeof(glm::vec2);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding for the z_data
    bindings[1].binding = 1;
    bindings[1].stride = sizeof(float);
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attrs{};

    // Attribute for grid_xy_data
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[0].offset = 0;

    // Attribute for z_data
    attrs[1].location = 1;
    attrs[1].binding = 1;
    attrs[1].format = VK_FORMAT_R32_SFLOAT;
    attrs[1].offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
    };

    VkViewport{
        0.0f,
        0.0f,
        static_cast<float>(_swap_chain_extent.width),
        static_cast<float>(_swap_chain_extent.height),
        0.0f,
        1.0f
    };
    VkRect2D{
        VkOffset2D{ 0, 0 },
        _swap_chain_extent
    };

    std::vector dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    };
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable    = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };


    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
    };

    VkPushConstantRange range{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(SurfacePushConstants),
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range,
    };
    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = shaderStages,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = nullptr,
    };

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &_swap_chain_surface_format.format,
        .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
    };

    graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

    if (vkCreateGraphicsPipelines(_device, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &_graphics_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    } 

    vkDestroyShaderModule(_device, shaderModule, nullptr);
#ifndef NDEBUG
    std::cout << "Created graphics pipeline" << std::endl;
#endif
}

[[nodiscard]]
VkShaderModule VulkanRenderer::create_shader_module(const std::vector<char> &code) const {
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    VkShaderModule shaderModule{};
    if(vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

void VulkanRenderer::record_command_buffer(uint32_t imageIndex, AppInfo& info) {
    std::cout << "Started record cmd buffer" << std::endl;

    VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (vkBeginCommandBuffer(_command_buffers[_current_frame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer");
    }

    // Transition the image layout for rendering
    transition_image_layout(
        _swap_chain_images[imageIndex],
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        0,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    // Transition depth image
    transition_image_layout(
        _depth_image,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        0,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
    );

    // Set up the color attachment
    VkClearValue clearColor = {
        .color = {
            {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    VkClearValue depthClearColor = {
        .depthStencil{
            .depth = 1.0f,
            .stencil = 0,
        }
    };

    VkRenderingAttachmentInfo depthAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = _depth_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = depthClearColor,
    };

    VkRenderingAttachmentInfo attachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = _swap_chain_image_views[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor
    };

    // Set up the rendering info
    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .offset = { 0, 0 }, .extent = _swap_chain_extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo,
    };

    // Begin rendering
    vkCmdBeginRendering(_command_buffers[_current_frame], &renderingInfo);

    vkCmdBindPipeline(_command_buffers[_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphics_pipeline);
    vkCmdBindDescriptorSets(_command_buffers[_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);

    VkViewport viewport{
        0.0f,
        0.0f,
        static_cast<float>(_swap_chain_extent.width),
        static_cast<float>(_swap_chain_extent.height),
        0.0f,
        1.0f,
    };
    VkRect2D scissor{
        VkOffset2D{0, 0},
        _swap_chain_extent
    };

    vkCmdSetViewport(_command_buffers[_current_frame], 0, 1, &viewport);
    vkCmdSetScissor(_command_buffers[_current_frame], 0, 1, &scissor);

    // Bind all buffers to their respective location, binding and offsets
    std::array<VkBuffer, 2> buffers = { _grid_xy_data, _bathymetry_z_data};
    VkDeviceSize offsets[] = {0, 0};

    vkCmdBindVertexBuffers(_command_buffers[_current_frame], 0, static_cast<uint32_t>(buffers.size()), buffers.data(), offsets);
    vkCmdBindIndexBuffer(_command_buffers[_current_frame], _index_data, 0, VK_INDEX_TYPE_UINT32);

    float temp_max{_frame_data.zMax};
    float temp_min{_frame_data.zMin};

    _frame_data.zMax = _metadata.bathymetryZMax;
    _frame_data.zMin = _metadata.bathymetryZMin;
    _frame_data.renderMode = _settings.bathymetryRenderMode;
    _frame_data.zScale = _settings.bathymetryZScale;

    vkCmdPushConstants(_command_buffers[_current_frame], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SurfacePushConstants), &_frame_data);
    vkCmdDrawIndexed(_command_buffers[_current_frame], _indices_size, 1, 0, 0, 0);


    vkCmdBindVertexBuffers(_command_buffers[_current_frame], 1, 1, &_frame_z_data, &offsets[0]);
    
    _frame_data.zMax = temp_max;
    _frame_data.zMin = temp_min;
    _frame_data.renderMode = _settings.surfaceRenderMode;
    _frame_data.zScale = _settings.surfaceZScale;
    vkCmdPushConstants(_command_buffers[_current_frame], pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SurfacePushConstants), &_frame_data);
    vkCmdDrawIndexed(_command_buffers[_current_frame], _indices_size, 1, 0, 0, 0);

    draw_imgui(info);

    // End rendering
    vkCmdEndRendering(_command_buffers[_current_frame]);

    // Transition the image layout for presentation
    transition_image_layout(
        _swap_chain_images[imageIndex],
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
    );

    vkEndCommandBuffer(_command_buffers[_current_frame]);

    std::cout << "Ended record cmd buffer" << std::endl;
}

void VulkanRenderer::create_command_pools() {
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _graphics_index
    };

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_command_pools[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool " + i);
        }
    }


#ifndef NDEBUG
    std::cout << "Created command pools" << std::endl;
#endif
}

void VulkanRenderer::create_command_buffers() {
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = _command_pools[i],
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(_device, &allocInfo, &_command_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffer");
        }
    }


#ifndef NDEBUG
    std::cout << "Allocated command buffers" << std::endl;
#endif
}

void VulkanRenderer::transition_image_layout(
    VkImage image,
    VkImageAspectFlags aspectMask,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags2 srcAccessMask,
    VkAccessFlags2 dstAccessMask,
    VkPipelineStageFlags2 srcStageMask,
    VkPipelineStageFlags2 dstStageMask)
{
    VkImageMemoryBarrier2 barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkDependencyInfo dependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    vkCmdPipelineBarrier2(
        _command_buffers[_current_frame],
        &dependencyInfo);
}

void VulkanRenderer::create_sync_objects() {
    VkSemaphoreCreateInfo semInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(_device, &semInfo, nullptr, &_present_complete_semaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create present complete semaphore " + i);
        }

        if (vkCreateSemaphore(_device, &semInfo, nullptr, &_render_finished_semaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render finished semaphore " + i);
        }
    }

    VkFenceCreateInfo fenInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(_device, &fenInfo, nullptr, &_draw_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create draw fence " + i);
        }
    }

#ifndef NDEBUG
    std::cout << "Created sync objects" << std::endl;
#endif
}

void VulkanRenderer::cleanup_swap_chain() {
    _swap_chain_image_views.clear();
    _swap_chain = nullptr;
}

void VulkanRenderer::recreate_swap_chain() {
    vkDeviceWaitIdle(_device);

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
}

uint32_t VulkanRenderer::find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Query memory properties
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physical_device, &memProperties);

    for (uint32_t i = 0; memProperties.memoryTypeCount; i++){
        if (
            (typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties
        ) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

// Helper function to copy data from a source buffer(usually a host visible memory) to
// a destination buffer (usually a device local memory)
void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _command_pools[_current_frame];
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_graphics_queue);

    vkFreeCommandBuffers(_device, _command_pools[_current_frame], 1, &commandBuffer);
}

// Helper function to create a buffer with allocated memory
void VulkanRenderer::create_buffer(
    VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
    VkDeviceMemory& bufferMemory
) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

// Function to initialize the VkBuffer structures for a simulation
void VulkanRenderer::create_buffers(
    const std::vector<glm::vec2>& grid,
    const std::vector<float>& bathymetryZ,
    const std::vector<Triangle>& triangles
 ) {
    // VkBuffer _frame_z_data creation
    VkDeviceSize size = sizeof(float) * bathymetryZ.size();
    create_buffer(size,
                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                  | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  _frame_z_data, _frame_z_data_mem);

    create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  _staging_buffer, _staging_buffer_mem);

    // VkBuffer _bathymetry_z_data creation
    create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                  | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  _bathymetry_z_data, _bathymetry_z_data_mem);

    upload_buffer(
        _bathymetry_z_data,
        sizeof(float)*bathymetryZ.size(),
        bathymetryZ.data()
    );

    // VkBuffer _grid_xy_data creation
    size = sizeof(glm::vec2) * grid.size();
    create_buffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                  | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  _grid_xy_data, _grid_xy_data_mem);
    upload_buffer(
        _grid_xy_data,
        sizeof(glm::vec2)*grid.size(),
        grid.data()
    );

    // VkBuffer _index_data creation
    size = sizeof(Triangle) * triangles.size();
    _indices_size = size / sizeof(uint32_t);

    create_buffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                  | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  _index_data, _index_data_mem);

    upload_buffer(
        _index_data,
        sizeof(Triangle)*triangles.size(),
        triangles.data()
    );

    return;
}

void VulkanRenderer::update_frame_z_data(Frame& frame) {
    auto size = sizeof(float) * frame.z_data.size();

    void* data;
    vkMapMemory(_device, _staging_buffer_mem, 0, size, 0, &data);
    memcpy(data, frame.z_data.data(), (size_t) size);
    vkUnmapMemory(_device, _staging_buffer_mem);

    copyBuffer(_staging_buffer, _frame_z_data, size);
}

void VulkanRenderer::update_scene(float zmin, float zmax, float dt) {
#ifdef NDEBUG
    std::cerr << "Started updated scene\n";
#endif
    camera.update(dt);

    _frame_data.zMin = zmin;
    _frame_data.zMax = zmax;

    UniformBufferObject ubo{};

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)_swap_chain_extent.width / (float)_swap_chain_extent.height, 0.1f, 1000.0f);

    proj[1][1] *= -1;

    glm::mat4 mvp = proj * view * model;

    ubo.mvp = mvp;

    memcpy(uniformBufferMapped, &ubo, sizeof(ubo));

#ifdef NDEBUG
    std::cerr << "Ended update scene\n";
#endif
}

Camera& VulkanRenderer::get_camera() {
    return camera;
}

void VulkanRenderer::create_descriptor_set_layout() {
#ifndef NDEBUG
    std::cout << "Created Descriptor Set Layout" << std::endl;
#endif

    VkDescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };

    if(vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void VulkanRenderer::create_uniform_buffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    create_buffer(
        bufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uniformBuffer,
        uniformBufferMemory
    );

    vkMapMemory(
        _device,
        uniformBufferMemory,
        0,
        bufferSize,
        0,
        &uniformBufferMapped
    );
}


void VulkanRenderer::create_descriptor_pool() {
    VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1
    };
    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    if(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void VulkanRenderer::create_descriptor_sets() {
    descriptorSets.resize(1);
    std::vector<VkDescriptorSetLayout> layouts(1, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };

    if (vkAllocateDescriptorSets(_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    for (size_t i = 0; i < 1; i++) {
        VkDescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffer,
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };
        VkWriteDescriptorSet   descriptorWrite{
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = descriptorSets[i],
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo     = &bufferInfo
        };

        vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
    }
}

void VulkanRenderer::init_imgui() {
    // Create imgui descriptor pool
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 0;
    
    for (VkDescriptorPoolSize& pool_size : pool_sizes) {
        pool_info.maxSets += pool_size.descriptorCount;
    }
    
    pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    if( vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imgui_descriptor_pool) != VK_SUCCESS ) {
        throw std::runtime_error("Failed to create imgui pool");
    }

    // ImGui initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _physical_device;
    init_info.Device = _device;
    init_info.QueueFamily = _graphics_index;
    init_info.Queue = _graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = _imgui_descriptor_pool;
    init_info.MinImageCount = static_cast<uint32_t>(_swap_chain_images.size());
    init_info.ImageCount = static_cast<uint32_t>(_swap_chain_images.size());
    init_info.Allocator = nullptr;
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    VkFormat format = _swap_chain_surface_format.format;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
    };

    if (_window) {
        std::cout << "_window is valid" << std::endl;
    }else {
        std::cout << "_window is nullptr" << std::endl;
    }
    

    bool glfw_ok = ImGui_ImplGlfw_InitForVulkan(_window, true);
    if (!glfw_ok) {
        throw std::runtime_error("Failed ImGui GLFW init");
    }
    
    std::cout << "ImGui GLFW initialized" << std::endl;

    bool vk_ok = ImGui_ImplVulkan_Init(&init_info);
    if (!vk_ok) {
        throw std::runtime_error("Failed ImGui Vulkan init");
    }
    
    std::cout << "ImGui Vulkan initialized" << std::endl;

#ifndef NDEBUG
    std::cout << "Initialized imgui" << std::endl;
#endif
}

void VulkanRenderer::draw_imgui(AppInfo& info) {
    // Camera data
    float yaw{camera.getYaw()};
    float pitch{camera.getPitch()};
    float distance{camera.getDistance()};
    
    // Simulation data
    float duration{info.get_frame_duration()};
    int qtd_frames{info.get_qtd_frames()};
    int count{info.get_frame_count()};
    float ratio{static_cast<float>(count) / static_cast<float>(qtd_frames)};

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Shallow Water Equations - Vulkan Renderer");

    ImGui::Text("Simulation data");
    ImGui::Text("Rows: %d", _metadata.rows);
    ImGui::Text("Cols: %d", _metadata.cols);
    ImGui::Text("Bathymetry Z Min: %f", _metadata.bathymetryZMin);
    ImGui::Text("Bathymetry Z Max: %f", _metadata.bathymetryZMax);
    ImGui::Text("X Extent: %f", _metadata.xExtent);
    ImGui::Text("Y Extent: %f", _metadata.yExtent);

    ImGui::Separator();

    ImGui::Text("Frame rate: %d FPS", info.get_fps());
    ImGui::Text("Amount of simulation frames: %d", info.get_qtd_frames());
    ImGui::Text("Current simulation frame: %d", info.get_frame_count());
    ImGui::Text("Simulation Progress");
    ImGui::InputFloat("Minimum time on each simulation frame", &duration);
    ImGui::ProgressBar(ratio);

    ImGui::Separator();
    
    ImGui::Text("Camera settings");
    ImGui::InputFloat("Yaw", &yaw);
    ImGui::InputFloat("Pitch", &pitch);
    ImGui::InputFloat("Distance", &distance);

    ImGui::Separator();
    
    ImGui::Text("Rendering settings");
    ImGui::InputFloat("Surface Z Scale", &_settings.surfaceZScale);
    ImGui::InputFloat("Bathymetry Z Scale", &_settings.bathymetryZScale);
    ImGui::InputInt("Surface Render Mode", &_settings.surfaceRenderMode);
    ImGui::InputInt("Bathymetry Render Mode", &_settings.bathymetryRenderMode);

    ImGui::End();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), _command_buffers[_current_frame]);

    info.set_frame_duration(duration);
    camera.setYaw(yaw);
    camera.setPitch(pitch);
    camera.setDistance(distance);
}

void VulkanRenderer::cleanup_imgui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanRenderer::upload_buffer(
    VkBuffer dst,
    VkDeviceSize size,
    const void* data
)
{
    VkBuffer staging;
    VkDeviceMemory stagingMemory;

    create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging,
        stagingMemory
    );


    void* mapped;

    vkMapMemory(
        _device,
        stagingMemory,
        0,
        size,
        0,
        &mapped
    );

    memcpy(mapped, data, size);

    vkUnmapMemory(
        _device,
        stagingMemory
    );


    copyBuffer(
        staging,
        dst,
        size
    );


    vkDestroyBuffer(
        _device,
        staging,
        nullptr
    );

    vkFreeMemory(
        _device,
        stagingMemory,
        nullptr
    );
}

void VulkanRenderer::create_depth_image()
{
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = VK_FORMAT_D32_SFLOAT;
    info.extent = {
        _swap_chain_extent.width,
        _swap_chain_extent.height,
        1
    };
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(_device, &info, nullptr, &_depth_image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image.");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, _depth_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_depth_image_memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate depth image memory.");
    }

    vkBindImageMemory(
        _device,
        _depth_image,
        _depth_image_memory,
        0);

#ifndef NDEBUG
    std::cerr << "Created depth image\n";
#endif
}

void VulkanRenderer::create_depth_image_view()
{
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = _depth_image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = VK_FORMAT_D32_SFLOAT;

    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(_device, &info, nullptr, &_depth_image_view) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image view.");
    }
#ifndef NDEBUG
    std::cerr << "Created depth image view\n";
#endif
}