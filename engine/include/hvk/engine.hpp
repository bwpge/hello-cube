#pragma once

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "hvk/allocator.hpp"
#include "hvk/camera.hpp"
#include "hvk/core.hpp"
#include "hvk/descriptor_utils.hpp"
#include "hvk/depth_buffer.hpp"
#include "hvk/pipeline_builder.hpp"
#include "hvk/resource_manager.hpp"
#include "hvk/scene.hpp"
#include "hvk/shader.hpp"
#include "hvk/texture.hpp"
#include "hvk/timer.hpp"
#include "hvk/buffer.hpp"
#include "hvk/vk_context.hpp"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace hvk {

struct PushConstants {
    glm::mat4 model{};
    glm::mat4 normal_transform{};
};

enum class BufferingMode : usize {
    None = 1u,
    Double,
    Triple,
};

struct FrameData {
    vk::UniqueSemaphore present_semaphore{};
    vk::UniqueSemaphore render_semaphore{};
    vk::UniqueFence render_fence{};
    vk::UniqueCommandPool cmd_pool{};
    vk::UniqueCommandBuffer cmd{};
    Buffer camera_ubo{};
    Buffer object_ssbo{};
    vk::DescriptorSet descriptor{};
    // NOTE: this descriptor set is freed by the owning pool, and since we are
    //   not using VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, we don't
    //   need to explicitly destroy them in the cleanup method
};

struct WindowData {
    std::string title{};
    i32 width{1600};
    i32 height{800};
    i32 start_x{};
    i32 start_y{};
    bool is_fullscreen{};
    GLFWwindow* handle{nullptr};
    GLFWmonitor* monitor{nullptr};
    const GLFWvidmode* mode{nullptr};
};

class Engine {
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    explicit Engine(
        std::string_view title,
        i32 width,
        i32 height,
        BufferingMode buffering = BufferingMode::None
    )
        : _max_frames_in_flight(static_cast<usize>(buffering)),
          _window{std::string{title}, width, height},
          _frames{_max_frames_in_flight} {}

    Engine() = delete;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;
    ~Engine() = default;

    void init();
    void run();
    void update(double dt);
    void render();
    void cleanup();

    void cycle_pipeline();
    void toggle_fullscreen();
    void on_resize();
    void on_window_resize(i32 width, i32 height);
    void on_window_move(i32 x, i32 y);
    void on_focus(bool focused);
    void on_mouse_move(glm::dvec2 pos);
    void on_scroll(double dx, double dy);
    void on_key_press(i32 keycode, i32 mods);

private:
    void init_glfw();
    void init_vulkan();
    void create_buffers();
    void create_scene();
    void init_commands();
    void init_renderpass();
    void create_framebuffers();
    void init_descriptors();
    void create_pipelines();
    void create_sync_obj();
    void recreate_swapchain();
    void destroy_swapchain();

    bool _is_init{};
    bool _focused{};
    bool _resized{};
    usize _frame_count{};
    usize _frame_idx{};
    usize _max_frames_in_flight{2};
    WindowData _window{};
    Timer _timer{};
    Camera _camera{};
    glm::dvec2 _cursor{};
    Scene _scene{};
    Buffer _scene_ubo{};
    DescriptorSetBindingMap _frame_bindings{};
    DescriptorSetBindingMap _texture_bindings{};

    UploadContext _upload_ctx{};
    DepthBuffer _depth_buffer{};
    std::vector<FrameData> _frames{};
    vk::UniqueRenderPass _render_pass{};
    std::vector<vk::UniqueFramebuffer> _framebuffers{};
    vk::UniqueDescriptorPool _desc_pool{};
    vk::UniqueDescriptorSetLayout _global_desc_set_layout{};
    vk::UniqueDescriptorSetLayout _texture_set_layout{};
    vk::DescriptorSet _texture_set{};
    usize _pipeline_idx{};
    GraphicsPipeline _pipelines{};
};

}  // namespace hvk
