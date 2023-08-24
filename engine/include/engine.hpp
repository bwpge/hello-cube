#pragma once

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "core.hpp"
#include "debug_utils.hpp"
#include "shader.hpp"
#include "mesh.hpp"

struct GLFWwindow;

namespace hc {

struct QueueFamily {
    u32 graphics;
    u32 present;
};

class Engine {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit Engine(std::string_view title, i32 width, i32 height)
        : _title{title},
          _width{width},
          _height{height} {}

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void init();
    void run();
    void render();
    void cleanup();
    void resized();

private:
    void init_vulkan();
    void create_swapchain();
    void create_vertex_buffers();
    void init_commands();
    void init_renderpass();
    void create_framebuffers();
    void create_pipelines();
    void init_sync_obj();
    void create_instance();
    void create_surface();
    void create_device();
    void load_shaders();
    void recreate_swapchain();
    void destroy_swapchain();
    u32 find_memory_type(u32 filter, vk::MemoryPropertyFlags properties);

    bool _is_init{};
    bool _resized{};
    i32 _frame_number{};
    std::string _title{};
    i32 _width{1600};
    i32 _height{800};
    GLFWwindow* _window{nullptr};
    const std::vector<Vertex> _vertices{
        {{0.0f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    };

    vk::UniqueInstance _instance{};
    vk::UniqueDebugUtilsMessengerEXT _messenger{};
    vk::PhysicalDevice _gpu{};
    vk::UniqueDevice _device{};
    ShaderMap _shaders{};
    vk::Queue _graphics_queue{};
    vk::UniqueSurfaceKHR _surface{};
    QueueFamily _indices{};
    vk::Format _swapchain_format{};
    vk::Extent2D _swapchain_extent{};
    vk::UniqueSwapchainKHR _swapchain{};
    std::vector<vk::Image> _swapchain_images{};
    std::vector<vk::UniqueImageView> _swapchain_image_views{};
    vk::UniqueBuffer _vertex_buffer{};
    vk::UniqueDeviceMemory _vertex_buffer_mem{};
    vk::UniqueCommandPool _cmd_pool{};
    vk::UniqueCommandBuffer _cmd_buffer{};
    vk::UniqueRenderPass _render_pass{};
    std::vector<vk::UniqueFramebuffer> _framebuffers{};
    vk::UniqueSemaphore _present_semaphore{}, _render_semaphore{};
    vk::UniqueFence _render_fence{};
    vk::UniquePipelineLayout _layout{};
    vk::UniquePipeline _graphics_pipeline{};
};

}  // namespace hc
