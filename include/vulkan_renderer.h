#include "renderer.h"

#include <memory>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer() {};
    void init() override;
    void draw_triangle() override;
    void draw_rectangle() override;

private:
    void create_instance();
    void setup_debug_messenger();
    void pick_physical_device();
    bool is_device_suitable(vk::raii::PhysicalDevice const & physicalDevice);
    void create_logical_device();

    std::vector<const char*> get_required_instance_extensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    std::unique_ptr<vk::raii::Context> _context = std::make_unique<vk::raii::Context>();
    std::unique_ptr<vk::raii::Instance> _instance = nullptr;
    std::unique_ptr<vk::raii::PhysicalDevice> _physical_device = nullptr;
    std::unique_ptr<vk::raii::Device> _device = nullptr;
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> _debug_messenger = nullptr;
    std::unique_ptr<vk::PhysicalDeviceFeatures> _device_features = nullptr;
    std::unique_ptr<vk::raii::Queue> _graphics_queue = nullptr;

    const std::vector<char const*> _validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enable_validation_layers = true;
#else
    const bool enable_validation_layers = false;
#endif
};
