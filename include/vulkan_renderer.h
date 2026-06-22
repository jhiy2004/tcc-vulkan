#include "renderer.h"
#include "window.h"

#include "volk.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <array>
#include <cstdint>


#include "loader.h"
#include "camera.h"

constexpr static uint32_t MAX_FRAMES_IN_FLIGHT{2};

// Deve ser um multiplo de 16 para manter o alinhamento
struct UniformBufferObject
{
    glm::mat4 mvp;
    float zMin;
    float zMax;

    uint16_t colorByHeight;
};

class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer(){};
    void init(IWindow* window,
              const std::vector<glm::vec2>& grid,
              const std::vector<float>& bathymetryZ,
              const std::vector<Triangle>& triangles
              ) override;
    void draw() override;
    void update_frame_z_data(Frame& frame) override;
    void update_scene(float zmin, float zmax) override;

    Camera& get_camera() override;
private:
    void create_instance();
    void setup_debug_messenger();
    void pick_physical_device();
    bool is_device_suitable(VkPhysicalDevice const & physicalDevice);
    void create_logical_device();
    void create_surface();
    void create_swap_chain();
    void create_graphics_pipeline();
    void create_descriptor_set_layout();

    Camera camera;

    VkSurfaceFormatKHR choose_swap_surface_format(std::vector<VkSurfaceFormatKHR> const &availableFormats);
    VkPresentModeKHR choose_swap_present_mode(std::vector<VkPresentModeKHR> const &availablePresentModes);
    VkExtent2D choose_swap_extent(GLFWwindow* window, VkSurfaceCapabilitiesKHR const &capabilities);
    uint32_t choose_swap_min_image_count(VkSurfaceCapabilitiesKHR const &surfaceCapabilities);
    void create_image_views();

    std::vector<const char*> get_required_instance_extensions();
    static VkBool32 debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData
    );

    VkShaderModule create_shader_module(const std::vector<char> &code) const;
    void record_command_buffer(uint32_t imageIndex);
    void create_command_pools();
    void create_command_buffers();

    void transition_image_layout(
        uint32_t imageIndex,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkAccessFlags2 srcAccessMask,
        VkAccessFlags2 dstAccessMask,
        VkPipelineStageFlags2 srcStageMask,
        VkPipelineStageFlags2 dstStageMask
    );

    void create_sync_objects();
    void recreate_swap_chain();
    void cleanup_swap_chain();


    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void create_buffer(
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
        VkDeviceMemory& bufferMemory
    );
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void create_buffers(
        const std::vector<glm::vec2>& grid,
        const std::vector<float>& bathymetryZ,
        const std::vector<Triangle>& triangles
    );

    void create_uniform_buffers();
    void create_descriptor_pool();
    void create_descriptor_sets();

    // Changes every frame
    VkBuffer _frame_z_data{};
    VkBuffer _staging_buffer{};

    // Only changes one time per simulation
    VkBuffer _bathymetry_z_data{};
    VkBuffer _grid_xy_data{};
    VkBuffer _index_data{};

    VkDeviceMemory _staging_buffer_mem{};
    VkDeviceMemory _frame_z_data_mem{};
    VkDeviceMemory _bathymetry_z_data_mem{};
    VkDeviceMemory _grid_xy_data_mem{};
    VkDeviceMemory _index_data_mem{};

    // Indices information
    uint32_t _indices_size{};

    bool _framebuffer_resized{false};
    std::vector<const char*> required_device_extension = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    GLFWwindow* _window{nullptr};
    VkInstance _instance{};
    VkPhysicalDevice _physical_device{};
    VkDevice _device{};
    VkDebugUtilsMessengerEXT _debug_messenger{};
    VkPhysicalDeviceFeatures _device_features{};
    VkQueue _graphics_queue{};
    std::uint32_t _graphics_index = 0;
    VkSurfaceKHR _surface{};
    VkPipeline _graphics_pipeline{};

    std::array<VkCommandPool, MAX_FRAMES_IN_FLIGHT> _command_pools;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> _command_buffers;

    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> _present_complete_semaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> _render_finished_semaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> _draw_fences;

    VkSwapchainKHR _swap_chain{};
    std::vector<VkImage> _swap_chain_images{};
    VkSurfaceFormatKHR   _swap_chain_surface_format{};
    VkExtent2D           _swap_chain_extent{};
    std::vector<VkImageView> _swap_chain_image_views{};

    VkDescriptorSetLayout descriptorSetLayout{};
    VkPipelineLayout pipelineLayout{};
    VkDescriptorPool descriptorPool{};
    std::vector<VkDescriptorSet> descriptorSets{};

    VkBuffer                uniformBuffer{};
    VkDeviceMemory          uniformBufferMemory{};
    void                   *uniformBufferMapped{nullptr};

    const std::vector<char const*> _validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t _current_frame{0};

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif
};
