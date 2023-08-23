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

private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_renderpass();
    void init_framebuffers();
    void init_sync_obj();
    void create_instance();
    void create_surface();
    void create_device();

    bool _is_init{false};
    i32 _frame_number{0};
    std::string _title{};
    i32 _width{1600};
    i32 _height{800};
    GLFWwindow* _window{nullptr};

    vk::UniqueInstance _instance{};
    vk::UniqueDebugUtilsMessengerEXT _messenger{};
    vk::PhysicalDevice _gpu{};
    vk::UniqueDevice _device{};
    vk::Queue _graphics_queue{};
    vk::UniqueSurfaceKHR _surface{};
    QueueFamily _indices{};
    vk::Format _swapchain_format{};
    vk::Extent2D _swapchain_extent{};
    vk::UniqueSwapchainKHR _swapchain{};
    std::vector<vk::Image> _swapchain_images{};
    std::vector<vk::UniqueImageView> _swapchain_image_views{};
    vk::UniqueCommandPool _cmd_pool{};
    vk::UniqueCommandBuffer _cmd_buffer{};
    vk::UniqueRenderPass _render_pass{};
    std::vector<vk::UniqueFramebuffer> _framebuffers{};
    vk::UniqueSemaphore _present_semaphore{}, _render_semaphore{};
    vk::UniqueFence _render_fence{};
};

}  // namespace hc
