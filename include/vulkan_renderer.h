#include "renderer.h"

#include <memory>
#include <GLFW/glfw3.h>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer() {};
    void init(IWindow* window) override;
    void draw_triangle() override;
    void draw_rectangle() override;

private:
    void create_instance();
    void setup_debug_messenger();
    void pick_physical_device();
    bool is_device_suitable(vk::raii::PhysicalDevice const & physicalDevice);
    void create_logical_device();
    void create_surface(GLFWwindow* window);
    void create_swap_chain(GLFWwindow* window);
    void create_graphics_pipeline();

    vk::SurfaceFormatKHR choose_swap_surface_format(std::vector<vk::SurfaceFormatKHR> const &availableFormats);
    vk::PresentModeKHR choose_swap_present_mode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
    vk::Extent2D choose_swap_extent(GLFWwindow* window, vk::SurfaceCapabilitiesKHR const &capabilities);
    uint32_t choose_swap_min_image_count(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    void create_image_views();

    std::vector<const char*> get_required_instance_extensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    vk::raii::ShaderModule create_shader_module(const std::vector<char> &code) const;
    void record_command_buffer(uint32_t imageIndex);
    void create_command_pool();
    void create_command_buffer();

    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask
    );

    void create_sync_objects();


    std::vector<const char*> required_device_extension = {vk::KHRSwapchainExtensionName};

    std::unique_ptr<vk::raii::Context> _context = std::make_unique<vk::raii::Context>();
    std::unique_ptr<vk::raii::Instance> _instance = nullptr;
    std::unique_ptr<vk::raii::PhysicalDevice> _physical_device = nullptr;
    std::unique_ptr<vk::raii::Device> _device = nullptr;
    std::unique_ptr<vk::raii::DebugUtilsMessengerEXT> _debug_messenger = nullptr;
    std::unique_ptr<vk::PhysicalDeviceFeatures> _device_features = nullptr;
    std::unique_ptr<vk::raii::Queue> _graphics_queue = nullptr;
    std::uint32_t _graphics_index = 0;
    std::unique_ptr<vk::raii::SurfaceKHR> _surface = nullptr;
    vk::raii::Pipeline       _graphics_pipeline = nullptr;

    vk::raii::CommandPool    _command_pool      = nullptr;
	vk::raii::CommandBuffer  _command_buffer    = nullptr;

    vk::raii::Semaphore _present_complete_semaphore = nullptr;
    vk::raii::Semaphore _render_finished_semaphore = nullptr;
    vk::raii::Fence _draw_fence = nullptr;

    vk::raii::SwapchainKHR _swap_chain = nullptr;
    std::vector<vk::Image> _swap_chain_images;
    vk::SurfaceFormatKHR   _swap_chain_surface_format;
    vk::Extent2D           _swap_chain_extent;
    std::vector<vk::raii::ImageView> _swap_chain_image_views;

    const std::vector<char const*> _validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enable_validation_layers = true;
#else
    const bool enable_validation_layers = false;
#endif
};
